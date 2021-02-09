#include <iostream>
#include <sstream>

#include "../../../../../common/misc/utility/HttpSender.h"
int main()
{
	std::string toSplit("192.168.192.107:80");
	std::istringstream ss(toSplit);
	std::string splitted;
	std::string addr = "";
	std::string port = "";
	std::string DATO = "[1612792280,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48]";
	int cnt = 0;

	while (std::getline(ss, splitted, ':')) {
		if (cnt == 0)
			addr = splitted;
		if (cnt == 1)
			port = splitted;
		if (cnt == 2)
		{
			addr += ":" + port;
			port = splitted;
		}
		cnt++;
	}
	
	::general::utility::HTTPResponse resp;

	::general::utility::HTTPClient   Sender(addr, port);
	resp = Sender.SendHttpPost("/dsdata/api/pushDafneData", "application/json;", DATO);
	std::cout << "ALEDEBUG: Sending data for graphics returned " << resp.ReturnCode;
	if (resp.ReturnCode != 201)
	{

	}
}