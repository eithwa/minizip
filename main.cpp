#include <iostream>
#include "MyMiniZip.h"
int main()
{
	MyMiniZip unZip;
	/*
	@��?? zip���]
	*/
	unZip.unZipPackageToLoacal("D:\\test.zip", "d:\\");
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());

	/*
	@ ??���Υ�??zip�]
	*/
	unZip.CompressToPackageZip("D:\\11111111", "D:\\test.zip");
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());
	system("pause");
	return 0;
}