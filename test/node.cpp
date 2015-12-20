/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-node.
 *
 * libbitcoin-node is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/node.hpp>

using namespace bc;
using namespace bc::chain;
using namespace bc::network;
using namespace bc::node;

struct low_thread_priority_fixture
{
    low_thread_priority_fixture()
    {
        set_thread_priority(thread_priority::lowest);
    }

    ~low_thread_priority_fixture()
    {
        set_thread_priority(thread_priority::normal);
    }
};

static void uninitchain(const char prefix[])
{
    boost::filesystem::remove_all(prefix);
}

static void initchain(const char prefix[])
{
    const size_t history_height = 0;
    const auto genesis = genesis_block();

    uninitchain(prefix);
    boost::filesystem::create_directories(prefix);
    initialize_blockchain(prefix);

    db_paths paths(prefix);
    db_interface interface(paths, { history_height });

    interface.start();
    interface.push(genesis);
}

// TODO: move construction expressions into BOOST_REQUIRE_NO_THROW.
BOOST_FIXTURE_TEST_SUITE(thread_tests, low_thread_priority_fixture)

// Just a basic test to get some coverage output.
BOOST_AUTO_TEST_CASE(node_test__construct_transaction_indexer__does_not_throw)
{
    threadpool threads;
    indexer index(threads);
    threads.stop();
    threads.join();
}

BOOST_AUTO_TEST_SUITE_END()

// TODO: move construction expressions into BOOST_REQUIRE_NO_THROW.
BOOST_FIXTURE_TEST_SUITE(node_tests, low_thread_priority_fixture)

BOOST_AUTO_TEST_CASE(node_test__construct_getx_responder__does_not_throw)
{
    // WARNING: file system side effect, use unique relative path.
    const static auto prefix = "node_test/construct_getx_responder";
    initchain(prefix);

    threadpool threads;
    blockchain_impl blockchain(threads, prefix);
    transaction_pool transactions(threads, blockchain, 2000, false);
    responder responder(blockchain, transactions, 0);

    blockchain.start();
    transactions.start();
    blockchain.stop();
    threads.stop();
    threads.join();

    // uninitchain(prefix);
}

BOOST_AUTO_TEST_CASE(node_test__construct_poller__does_not_throw)
{
    // WARNING: file system side effect, use unique relative path.
    const static auto prefix = "node_test/construct_poller";
    initchain(prefix);

    threadpool threads;
    blockchain_impl blockchain(threads, prefix);
    poller poller(blockchain, 0, 0);

    blockchain.start();
    blockchain.stop();
    threads.stop();
    threads.join();

    // uninitchain(prefix);
}

BOOST_AUTO_TEST_CASE(node_test__construct_session__does_not_throw)
{
    // WARNING: file system side effect, use unique relative path.
    const static auto prefix = "node_test/construct_session";
    initchain(prefix);

    threadpool threads;
    hosts hosts(threads);
    peer network(threads);
    handshake handshake(threads);
    protocol protocol(threads, hosts, handshake, network);
    blockchain_impl blockchain(threads, prefix);
    transaction_pool transactions(threads, blockchain, 2000, false);
    poller poller(blockchain, 0, 0);
    responder responder(blockchain, transactions, 0);
    inventory inventory(handshake, blockchain, transactions, 0);

    const auto noop_handler = [](std::error_code code)
    {
    };

    node::session session(protocol, blockchain, transactions, poller,
        responder, inventory, 0);

    blockchain.start();
    transactions.start();
    session.start(noop_handler);
    session.stop(noop_handler);
    blockchain.stop();
    threads.stop();
    threads.join();

    // uninitchain(prefix);
}

BOOST_AUTO_TEST_SUITE_END()
