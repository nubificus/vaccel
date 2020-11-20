#include "gtest/gtest.h"

extern "C" {
#include "backend.h"
#include "common.h"
#include "list.h"
}

#include <stdbool.h>
#include <string.h>


class BackendTests : public ::testing::Test {
	static int fini(struct vaccel_backend *backend)
	{
		(void)backend;
		return VACCEL_OK;
	} 
protected:
	struct vaccel_backend backend;
	
	void SetUp() override
	{
		backend.name = strdup("mock_backend");
		list_init_entry(&backend.entry);
		backend.fini = fini;
		backends_bootstrap();
	}

	void TearDown() override
	{
		list_unlink_entry(&backend.entry);
		free(backend.name);
		cleanup_backends();
	}
};

TEST_F(BackendTests, backend_init_null) {
	ASSERT_EQ(initialize_backend(NULL, "NULL backend"), VACCEL_EINVAL);
}

TEST_F(BackendTests, backend_init_null_name) {
	ASSERT_EQ(initialize_backend(&backend, NULL), VACCEL_EINVAL);
}

TEST(backend_init_not_bootstraped, not_ok) {
	struct vaccel_backend backend = {};
	ASSERT_EQ(initialize_backend(&backend, "mock_backend"), VACCEL_EBACKEND);
}

TEST_F(BackendTests, backend_init_values) {
	ASSERT_EQ(initialize_backend(&backend, "mock_backend"), VACCEL_OK);
	ASSERT_EQ(strcmp(backend.name, "mock_backend"), 0);
	ASSERT_TRUE(list_empty(&backend.ops));
}

TEST_F(BackendTests, backend_register_null) {
	ASSERT_EQ(register_backend(NULL), VACCEL_EINVAL);
}

TEST(backend_register_not_bootstraped, not_ok) {
	struct vaccel_backend backend = {};
	ASSERT_EQ(register_backend(&backend), VACCEL_EBACKEND);
}

TEST_F(BackendTests, backend_register_existing) {
	ASSERT_EQ(initialize_backend(&backend, "mock_backend"), VACCEL_OK);
	ASSERT_EQ(register_backend(&backend), VACCEL_OK);
	ASSERT_EQ(register_backend(&backend), VACCEL_EEXISTS);
}

TEST_F(BackendTests, backend_register_with_ops) {
}
