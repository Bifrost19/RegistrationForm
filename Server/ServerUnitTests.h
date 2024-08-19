#pragma once
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "Server.h"

sqlite3* dbTest;

TEST_CASE("Database creation and main database operations")
{
    // Use in-memory database for testing
    sqlite3_open(":memory:", &dbTest);

    // Test database creation
    CHECK(createDatabase(dbTest) == true);

    // Test adding a user
    CHECK(addUser("Andrew", "Andr0#", "andr@gmail.com", dbTest) == true);

    // Test duplicate username
    CHECK(addUser("Andrew", "#0rdnA", "Andrew@abv.bg", dbTest) == false);

    // Test duplicate email
    CHECK(addUser("John", "Jhn0#", "andr@gmail.com", dbTest) == false);

    // Test user login
    CHECK(loginUser("andr@gmail.com", "Andr0#", dbTest) == true);
    CHECK(loginUser("andr@gmail.com", "Bob14%", dbTest) == false);
}

TEST_CASE("Username and password updates")
{
    // Change username
    CHECK(changeUsername("Jack", "Andr0#", "andr@gmail.com", dbTest) == true);

    // Duplicate username
    CHECK(changeUsername("Jack", "Andr0#", "andr@gmail.com", dbTest) == false);

    // Test password change
    CHECK(changePassword("Jack15&", "andr@gmail.com", dbTest) == true);

    // Check login with updated password
    CHECK(loginUser("andr@gmail.com", "Jack15&", dbTest) == true);
}
