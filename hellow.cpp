#include <iostream>
#include <string>
#include <cstring>
using namespace std;

 int main(void) {
   char buffer[50];
   char buffer1[50];
   char buffer2[50];
   int num = 002;
   int num2 = 123456;
   int num3 = 898;
   if(num<9)
   int len = snprintf(buffer, sizeof(buffer), "0%d", num);
   else
   int len = snprintf(buffer, sizeof(buffer), "%d", num);
   cout << atoi(buffer) << endl;
   snprintf(buffer2, sizeof(buffer2), "%d", num3);
   strcat(buffer, buffer2);
   snprintf(buffer1, sizeof(buffer1), " and it's twice %d", num*2);
   strcat(buffer, buffer1);
   cout << buffer;
   return 0;
 }
