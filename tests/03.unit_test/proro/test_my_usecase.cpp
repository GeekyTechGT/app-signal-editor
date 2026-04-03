#include <gtest/gtest.h>
#include "proro/core/usecases/my_usecase.h"
#include "proro/ports/my_port.h"

using namespace myprj;
using namespace myprj::proro;

// Test double (stub repository)
class StubRepository : public IMyRepository {
public:
    myprj::Result save(const MyEntity& entity) override {
        saved_id = entity.id;
        return myprj::Result::ok();
    }
    std::vector<MyEntity> load_all() override { return {}; }

    std::string saved_id;
};

// Use case saves the entity through the repository
TEST(MyUseCaseTest, ExecuteSavesEntity) {
    StubRepository repo;
    MyUseCase uc(repo);

    auto result = uc.execute("id-001", "Test Entity");

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(repo.saved_id, "id-001");
}

// Use case returns error when id is empty
TEST(MyUseCaseTest, ExecuteFailsWithEmptyId) {
    StubRepository repo;
    MyUseCase uc(repo);

    auto result = uc.execute("", "Test Entity");

    EXPECT_FALSE(result.is_ok());
    EXPECT_FALSE(result.message.empty());
}
