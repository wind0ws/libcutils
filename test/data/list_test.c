#include "data/list.h"
#include "common_macro.h"

#define LOG_TAG "LIST_TEST"
#include "log/logger.h"

static void my_free_fn(void* ptr)
{
	if (ptr)
	{
		free(ptr);
	}
}

typedef struct 
{
	size_t count;
} my_list_iter_context;

static bool my_iter_list_fn(void* data, void* context)
{
	ASSERT(data);
	my_list_iter_context* p_ctx = (my_list_iter_context *)context;
	TLOGD(LOG_TAG, "list[%u] => %d", p_ctx->count, *(int*)data);
	++p_ctx->count;
	return true;
}

static int testcase()
{
	list_t *p_list = list_new(my_free_fn);
	ASSERT(p_list);
	ASSERT(list_is_empty(p_list));
	for (int i = 0; i < 10; ++i)
	{
		int* num_ptr = (int *)malloc(sizeof(int));
		ASSERT(num_ptr);
		*num_ptr = i;
		list_append(p_list, num_ptr);
	}
	ASSERT(list_length(p_list) == 10);
	ASSERT(!list_is_empty(p_list));
	int* prepend_ptr = (int*)malloc(sizeof(int));
	ASSERT(prepend_ptr);
	*prepend_ptr = -1;
	list_prepend(p_list, prepend_ptr);

	int *front_ptr = (int *)list_front(p_list);
	ASSERT(*front_ptr == -1);
	int* back_ptr = (int*)list_back(p_list);
	ASSERT(*back_ptr == 9);
	bool succeed = list_remove(p_list, back_ptr);
	ASSERT(succeed && list_length(p_list) == 10);

	back_ptr = (int*)list_back(p_list);
	ASSERT(*back_ptr == 8);
	list_node_t *back_node = list_back_node(p_list);
	int *new_back_ptr = (int*)malloc(sizeof(int));
	ASSERT(new_back_ptr);
	*new_back_ptr = 888;
	list_insert_after(p_list, back_node, new_back_ptr);
	
	my_list_iter_context ctx = { 0 };
	list_foreach(p_list, my_iter_list_fn, &ctx);
	TLOGD(LOG_TAG, "list_count=%zu", ctx.count);
	list_free(p_list);
	return 0;
}

int list_test()
{
	TLOGD(LOG_TAG, " --> list test begin");
	int code = testcase();
	TLOGD(LOG_TAG, " <-- list test end. %d", code);
	return code;
}
