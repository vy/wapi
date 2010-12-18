static void
getroutes(void)
{
	wapi_list_t list;

	bzero(&list, sizeof(wapi_list_t));
	if (wapi_get_routes(&list) >= 0)
	{
		wapi_route_info_t *ri;

		/* print route */
		for (ri = list.head.route; ri; ri = ri->next)
			printf(
				">> dest: %s, gw: %s, netmask: %s\n",
				inet_ntoa(ri->dest), inet_ntoa(ri->gw), inet_ntoa(ri->netmask));

		/* free routes */
		ri = list.head.route;
		while (ri)
		{
			wapi_route_info_t *tmpri;

			tmpri = ri->next;
			free(ri->ifname);
			free(ri);
			ri = tmpri;
		}
	}
}
