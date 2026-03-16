#include <stdio.h>
#include <string.h>

#include "../framework/test.h"

// ====================================================================
// Helpers
// ====================================================================

namespace {

// Path used for all round-trip tests. Cleaned up by each test that creates it.
const char* kTmpPath = "/tmp/test_fio_myos.tmp";

// Write content to kTmpPath via fopen/fwrite, close, return true on success.
bool write_tmp(const char* data, size_t len) {
  FILE* f = fopen(kTmpPath, "w");
  if (f == nullptr) {
    return false;
  }
  const size_t n = fwrite(data, 1, len, f);
  fclose(f);
  return n == len;
}

}  // namespace

// ====================================================================
// fopen / fclose
// ====================================================================

TEST(fio, fopen_nonexistent_read_returns_null) {
  const FILE* f = fopen("/tmp/does_not_exist_myos_12345.tmp", "r");
  ASSERT_NULL(f);
}

TEST(fio, fopen_write_creates_file) {
  FILE* f = fopen(kTmpPath, "w");
  ASSERT_NOT_NULL(f);
  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fopen_invalid_mode_returns_null) {
  const FILE* f = fopen(kTmpPath, "z");
  ASSERT_NULL(f);
}

// ====================================================================
// fwrite / fread round-trip
// ====================================================================

TEST(fio, fwrite_fread_roundtrip) {
  const char data[] = "hello, world";
  const size_t len = sizeof(data) - 1;

  ASSERT(write_tmp(data, len));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[32] = {0};
  const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  remove(kTmpPath);

  ASSERT_EQ(n, len);
  ASSERT_STR_EQ(buf, data);
}

TEST(fio, fread_sets_eof_at_end) {
  ASSERT(write_tmp("ab", 2));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[16];
  // Read all data.
  fread(buf, 1, 16, f);
  // Next read must trigger EOF.
  const size_t n = fread(buf, 1, 1, f);
  ASSERT_EQ(n, (size_t)0);
  ASSERT(feof(f));

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fwrite_returns_nmemb_on_success) {
  FILE* f = fopen(kTmpPath, "w");
  ASSERT_NOT_NULL(f);

  // Write 3 elements of size 4 = 12 bytes.
  const int vals[3] = {1, 2, 3};
  const size_t n = fwrite(vals, sizeof(int), 3, f);
  fclose(f);
  remove(kTmpPath);

  ASSERT_EQ(n, (size_t)3);
}

// ====================================================================
// fseek / ftell / rewind
// ====================================================================

TEST(fio, ftell_at_start_is_zero) {
  ASSERT(write_tmp("xyz", 3));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);
  ASSERT_EQ(ftell(f), 0L);
  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fseek_set_and_ftell) {
  ASSERT(write_tmp("abcde", 5));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  ASSERT_EQ(fseek(f, 3, SEEK_SET), 0);
  ASSERT_EQ(ftell(f), 3L);

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fseek_cur_advances_position) {
  ASSERT(write_tmp("abcde", 5));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  fseek(f, 2, SEEK_SET);
  fseek(f, 1, SEEK_CUR);
  ASSERT_EQ(ftell(f), 3L);

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, rewind_resets_to_start) {
  ASSERT(write_tmp("hello", 5));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[8] = {0};
  fread(buf, 1, 5, f);
  ASSERT_EQ(ftell(f), 5L);

  rewind(f);
  ASSERT_EQ(ftell(f), 0L);
  ASSERT_FALSE(feof(f));

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fseek_clears_eof) {
  ASSERT(write_tmp("hi", 2));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[8];
  fread(buf, 1, 8, f);  // Returns 2; positions at EOF but does not set the flag.
  fread(buf, 1, 1, f);  // Returns 0; now sets the EOF flag.
  ASSERT(feof(f));

  fseek(f, 0, SEEK_SET);
  ASSERT_FALSE(feof(f));

  fclose(f);
  remove(kTmpPath);
}

// ====================================================================
// fgetc / fputc
// ====================================================================

TEST(fio, fgetc_reads_bytes_in_order) {
  ASSERT(write_tmp("ABC", 3));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  ASSERT_EQ(fgetc(f), (int)'A');
  ASSERT_EQ(fgetc(f), (int)'B');
  ASSERT_EQ(fgetc(f), (int)'C');
  ASSERT_EQ(fgetc(f), EOF);

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fputc_writes_byte) {
  FILE* f = fopen(kTmpPath, "w");
  ASSERT_NOT_NULL(f);
  ASSERT_EQ(fputc('X', f), (int)'X');
  fclose(f);

  FILE* r = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(r);
  ASSERT_EQ(fgetc(r), (int)'X');
  fclose(r);
  remove(kTmpPath);
}

// ====================================================================
// fgets / fputs
// ====================================================================

TEST(fio, fgets_reads_line_with_newline) {
  ASSERT(write_tmp("line1\nline2\n", 12));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[32] = {0};
  const char* ret = fgets(buf, sizeof(buf), f);
  ASSERT_NOT_NULL(ret);
  ASSERT_STR_EQ(buf, "line1\n");

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fgets_returns_null_at_eof) {
  ASSERT(write_tmp("x", 1));

  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);

  char buf[16];
  fgets(buf, sizeof(buf), f);                    // Read "x".
  const char* ret = fgets(buf, sizeof(buf), f);  // Should be NULL.
  ASSERT_NULL(ret);

  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fputs_then_fgets_roundtrip) {
  FILE* f = fopen(kTmpPath, "w");
  ASSERT_NOT_NULL(f);
  fputs("hello\n", f);
  fclose(f);

  FILE* r = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(r);
  char buf[32] = {0};
  fgets(buf, sizeof(buf), r);
  fclose(r);
  remove(kTmpPath);

  ASSERT_STR_EQ(buf, "hello\n");
}

// ====================================================================
// ferror / fflush
// ====================================================================

TEST(fio, ferror_initially_zero) {
  ASSERT(write_tmp("x", 1));
  FILE* f = fopen(kTmpPath, "r");
  ASSERT_NOT_NULL(f);
  ASSERT_EQ(ferror(f), 0);
  fclose(f);
  remove(kTmpPath);
}

TEST(fio, fflush_returns_zero) {
  FILE* f = fopen(kTmpPath, "w");
  ASSERT_NOT_NULL(f);
  ASSERT_EQ(fflush(f), 0);
  fclose(f);
  remove(kTmpPath);
}
