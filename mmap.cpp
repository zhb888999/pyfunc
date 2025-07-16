#include <sys/mman.h>   // 包含 mmap 的头文件
#include <sys/stat.h>   // 包含 stat 的头文件
#include <fcntl.h>      // 包含 open 的头文件
#include <unistd.h>     // 包含 close 的头文件
#include <iostream>

int main() {
    int fd = open("example.txt", O_RDWR);  // 打开文件，以读写方式
    if (fd == -1) {
        perror("open");
        return 1;
    }

    ftruncate(fd,  1024);

    struct stat sb;
    if (fstat(fd, &sb) == -1) {            // 获取文件状态信息
        perror("fstat");
        close(fd);
        return 1;
    }

    void* addr = mmap(NULL, 1024,    // 映射文件到内存
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }


    std::cout << "!The first character is: " << std::endl;
    // 现在可以通过内存地址 addr 来访问或修改文件内容
    // 例如：将文件的第一个字符改为 'A'
    std::cout << addr << std::endl;
    static_cast<char*>(addr)[0] = 'A';

    std::cout << "The first character is: " << std::endl;

    // 同步内存中的更改到文件
    if (msync(addr, sb.st_size, MS_SYNC) == -1) {
        perror("msync");
    }

    // 解除映射
    if (munmap(addr, sb.st_size) == -1) {
        perror("munmap");
    }

    // 关闭文件
    close(fd);

    return 0;
}
