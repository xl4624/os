#include <algorithm.h>
#include <dirent.h>
#include <string.h>

#include "../framework/test.h"

// ====================================================================
// Mock getdents
// ====================================================================

namespace {

// Configurable mock state.
struct dirent g_mock_entries[DIR_BUF_SIZE];
int g_mock_count = 0;      // entries to return (-1 for error)
bool g_mock_fail = false;  // if true, getdents returns -1

void mock_reset() {
  g_mock_count = 0;
  g_mock_fail = false;
  memset(g_mock_entries, 0, sizeof(g_mock_entries));
}

void mock_add_entry(const char* name, uint8_t type, uint32_t size) {
  struct dirent* e = &g_mock_entries[g_mock_count];
  strncpy(e->d_name, name, sizeof(e->d_name) - 1);
  e->d_name[sizeof(e->d_name) - 1] = '\0';
  e->d_type = type;
  e->d_size = size;
  ++g_mock_count;
}

}  // namespace

extern "C" int getdents(const char* /*path*/, struct dirent* buf, unsigned int count) {
  if (g_mock_fail) {
    return -1;
  }
  int n = g_mock_count;
  n = std::min(n, static_cast<int>(count));
  memcpy(buf, g_mock_entries, static_cast<size_t>(n) * sizeof(struct dirent));
  return n;
}

// ====================================================================
// opendir()
// ====================================================================

TEST(dirent, opendir_null_path_returns_null) {
  mock_reset();
  DIR* d = opendir(nullptr);
  ASSERT_NULL(d);
}

TEST(dirent, opendir_getdents_failure_returns_null) {
  mock_reset();
  g_mock_fail = true;
  DIR* d = opendir("/nonexistent");
  ASSERT_NULL(d);
}

TEST(dirent, opendir_empty_directory) {
  mock_reset();
  DIR* d = opendir("/empty");
  ASSERT_NOT_NULL(d);
  ASSERT_NULL(readdir(d));
  ASSERT_EQ(closedir(d), 0);
}

TEST(dirent, opendir_succeeds) {
  mock_reset();
  mock_add_entry("file.txt", DT_REG, 100);
  DIR* d = opendir("/test");
  ASSERT_NOT_NULL(d);
  closedir(d);
}

// ====================================================================
// readdir()
// ====================================================================

TEST(dirent, readdir_null_returns_null) { ASSERT_NULL(readdir(nullptr)); }

TEST(dirent, readdir_single_entry) {
  mock_reset();
  mock_add_entry("hello.txt", DT_REG, 42);

  DIR* d = opendir("/test");
  ASSERT_NOT_NULL(d);

  struct dirent* e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, "hello.txt");
  ASSERT_EQ(e->d_type, DT_REG);
  ASSERT_EQ(e->d_size, static_cast<uint32_t>(42));

  ASSERT_NULL(readdir(d));
  closedir(d);
}

TEST(dirent, readdir_multiple_entries) {
  mock_reset();
  mock_add_entry(".", DT_DIR, 0);
  mock_add_entry("..", DT_DIR, 0);
  mock_add_entry("file.txt", DT_REG, 1024);
  mock_add_entry("dev", DT_DIR, 0);
  mock_add_entry("console", DT_CHR, 0);

  DIR* d = opendir("/");
  ASSERT_NOT_NULL(d);

  struct dirent* e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, ".");

  e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, "..");

  e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, "file.txt");
  ASSERT_EQ(e->d_type, DT_REG);
  ASSERT_EQ(e->d_size, static_cast<uint32_t>(1024));

  e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, "dev");
  ASSERT_EQ(e->d_type, DT_DIR);

  e = readdir(d);
  ASSERT_NOT_NULL(e);
  ASSERT_STR_EQ(e->d_name, "console");
  ASSERT_EQ(e->d_type, DT_CHR);

  ASSERT_NULL(readdir(d));
  closedir(d);
}

TEST(dirent, readdir_exhausted_returns_null_repeatedly) {
  mock_reset();
  mock_add_entry("a", DT_REG, 0);

  DIR* d = opendir("/");
  ASSERT_NOT_NULL(d);
  ASSERT_NOT_NULL(readdir(d));
  ASSERT_NULL(readdir(d));
  ASSERT_NULL(readdir(d));
  closedir(d);
}

// ====================================================================
// closedir()
// ====================================================================

TEST(dirent, closedir_null_returns_error) { ASSERT_EQ(closedir(nullptr), -1); }

TEST(dirent, closedir_returns_zero) {
  mock_reset();
  DIR* d = opendir("/test");
  ASSERT_NOT_NULL(d);
  ASSERT_EQ(closedir(d), 0);
}
