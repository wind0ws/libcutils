#include "file/file_util.h"
#include "mem/strings.h"
#include "file/file_iterator.h"

#define LOG_TAG "FILE_UTIL_TEST"
#include "log/logger.h"

static int pri_handle_file_info(file_info_t* p_info, void* user_data);

/* P0-4 回归测试: 验证 file_util_append_slash_on_path_if_needed 边界行为 */
static int pri_test_append_slash_boundary(void)
{
	int ret;

	/* 用例 1: 正常追加 — 缓冲足够,期望返回 0 且追加 '/' */
	{
		char buf[16] = "abc";
		ret = file_util_append_slash_on_path_if_needed(buf, sizeof(buf));
		if (ret != 0 || strcmp(buf, "abc/") != 0)
		{
			LOGE("case1 normal append fail: ret=%d, buf=\"%s\"", ret, buf);
			return -1;
		}
	}

	/* 用例 2: 缓冲恰好不足 — path_len+2 > folder_path_size,
	 * 期望返回 -3 且原数据未被破坏 (P0-4 修复前会静默覆盖末字符) */
	{
		char buf[5] = "abcd"; /* path_len=4, size=5, path_len+2=6 > 5 */
		ret = file_util_append_slash_on_path_if_needed(buf, sizeof(buf));
		if (ret != -3 || strcmp(buf, "abcd") != 0)
		{
			LOGE("case2 buffer-too-small fail: ret=%d, buf=\"%s\"", ret, buf);
			return -2;
		}
	}

	/* 用例 3: 已带斜杠 — 期望直接返回 0,不修改 */
	{
		char buf[16] = "abc/";
		ret = file_util_append_slash_on_path_if_needed(buf, sizeof(buf));
		if (ret != 0 || strcmp(buf, "abc/") != 0)
		{
			LOGE("case3 already-has-slash fail: ret=%d, buf=\"%s\"", ret, buf);
			return -3;
		}
	}

	/* 用例 4: NULL/空串/小缓冲 — 期望返回 -1 */
	{
		char buf[16] = "abc";
		if (file_util_append_slash_on_path_if_needed(NULL, sizeof(buf)) != -1)
		{
			LOGE("case4 NULL ptr should return -1");
			return -4;
		}
		char empty[8] = "";
		if (file_util_append_slash_on_path_if_needed(empty, sizeof(empty)) != -1)
		{
			LOGE("case4 empty string should return -1");
			return -4;
		}
		char small[2] = "a";
		if (file_util_append_slash_on_path_if_needed(small, sizeof(small)) != -1)
		{
			LOGE("case4 size<3 should return -1");
			return -4;
		}
	}

	/* 用例 5: 全空格 — 期望返回 -2 */
	{
		char buf[8] = "   ";
		ret = file_util_append_slash_on_path_if_needed(buf, sizeof(buf));
		if (ret != -2)
		{
			LOGE("case5 all-spaces should return -2: ret=%d", ret);
			return -5;
		}
	}

	LOGI("file_util_append_slash boundary tests all passed");
	return 0;
}

int file_util_test(void)
{
	char log_path[64] = { 0 };

	/* P0-4 边界测试先行 */
	int ret = pri_test_append_slash_boundary();
	if (ret != 0)
	{
		return ret;
	}

#ifdef _WIN32
	if (file_util_access(".\\mylog\\", F_OK))
	{
		file_util_mkdirs(".\\mylog\\");
	}
	strlcpy(log_path, ".\\mylog\\sub ", sizeof(log_path));
#else
	strlcpy(log_path, "./log/ ", sizeof(log_path));
#endif // _WIN32

	file_util_append_slash_on_path_if_needed(log_path, sizeof(log_path));
	if (file_util_access(log_path, F_OK))
	{
		file_util_mkdirs(log_path);
	}
#define TEST_FILE "CMakeCache.txt"
	if (file_util_access(TEST_FILE, F_OK) == 0)
	{
		FILE* fs = fopen(TEST_FILE, "rb");
		if (fs)
		{
			long file_size = file_util_get_size_by_fs(fs);
			LOGD_TRACE(" \"%s\" file size=%ld", TEST_FILE, file_size);
			fclose(fs);
		}
	}

	file_iterator_foreach("./", pri_handle_file_info, NULL);

	return 0;
}

static int pri_handle_file_info(file_info_t* p_info, void* user_data)
{
	if (FILE_ITERATOR_TYPE_DIR == p_info->type)
	{
		LOGD("it is dir => %s/%s", p_info->dir, p_info->name);
	}
	else if (FILE_ITERATOR_TYPE_FILE == p_info->type)
	{
		LOGD("it is file => %s/%s, size=%ld", p_info->dir, p_info->name, (long)(p_info->p_stat->st_size));
	}
	return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(file_util_test, "test file util");
