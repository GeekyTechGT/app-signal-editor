#include <gtest/gtest.h>
#include "sssss/core/domain/my_entity.h"

using namespace myprj::sssss;

// MyEntity can be created with valid data
TEST(MyEntityTest, CreateWithValidData) {
    auto entity = MyEntity::create("id-001", "Test Entity");
    EXPECT_EQ(entity.id, "id-001");
    EXPECT_EQ(entity.name, "Test Entity");
}

// MyEntity creation fails when id is empty
TEST(MyEntityTest, RejectsEmptyId) {
    EXPECT_THROW(MyEntity::create("", "Test Entity"), std::invalid_argument);
}

// MyEntity creation fails when name is empty
TEST(MyEntityTest, RejectsEmptyName) {
    EXPECT_THROW(MyEntity::create("id-001", ""), std::invalid_argument);
}
