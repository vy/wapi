static void
getifnames(void)
{
	wapi_list_t list;

	bzero(&list, sizeof(wapi_list_t));
	if (wapi_get_ifnames(&list) >= 0)
	{
		wapi_string_t *str;

		/* print ifnames */
		printf("ifnames:");
		for (str = list.head.string; str; str = str->next)
			printf(" %s", str->data);

		/* free ifnames */
		str = list.head.string;
		while (str)
		{
			wapi_string_t *tmp;

			tmp = str->next;
			free(str->data);
			free(str);
			str = tmp;
		}
	}
}
