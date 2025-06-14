


#include <common.h>
#include <exports.h>





extern void obc_main(void);
//int __attribute__((section(".text"))) start_obcboot (int argc, char * const argv[])

//int start_obcboot (int argc, char * const argv[])
//int __attribute__((section(".text"))) start_obcboot (int argc, char * const argv[])


int __attribute__((section(".text"))) start_obcboot (int argc, char * const argv[])
{
    app_startup(argv);

    obc_main();

    printf("ayan standalone\n");

    return 0;
}









