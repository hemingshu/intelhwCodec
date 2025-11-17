#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encode.h"
#include "gpu.h"
#include "colorspace.h"

/**
 * 读取一帧 YUV420P 数据
 * @param fp 文件指针
 * @param width 图像宽度
 * @param height 图像高度
 * @param y_data Y分量数据缓冲区
 * @param u_data U分量数据缓冲区
 * @param v_data V分量数据缓冲区
 * @return 成功返回0，失败返回-1
 */
int read_yuv420p_frame(FILE *fp, int width, int height, 
                       unsigned char *y_data, 
                       unsigned char *u_data, 
                       unsigned char *v_data) {
    if (!fp || !y_data || !u_data || !v_data) {
        fprintf(stderr, "Invalid parameters\n");
        return -1;
    }

    int y_size = width * height;
    int u_size = y_size / 4;
    int v_size = y_size / 4;

    // 读取 Y 分量
    if (fread(y_data, 1, y_size, fp) != y_size) {
        if (feof(fp)) {
            return -1; // 文件结束
        }
        fprintf(stderr, "Failed to read Y component\n");
        return -1;
    }

    // 读取 U 分量
    if (fread(u_data, 1, u_size, fp) != u_size) {
        fprintf(stderr, "Failed to read U component\n");
        return -1;
    }

    // 读取 V 分量
    if (fread(v_data, 1, v_size, fp) != v_size) {
        fprintf(stderr, "Failed to read V component\n");
        return -1;
    }

    return 0;
}

/**
 * 打开YUV文件并分配内存
 * @param input_file 输入文件路径
 * @param width 图像宽度
 * @param height 图像高度
 * @param fp 文件指针（输出参数）
 * @param y_data Y分量缓冲区（输出参数）
 * @param u_data U分量缓冲区（输出参数）
 * @param v_data V分量缓冲区（输出参数）
 * @return 成功返回0，失败返回-1
 */
int open_yuv_file(const char *input_file, int width, int height,
                  FILE **fp, unsigned char **y_data, 
                  unsigned char **u_data, unsigned char **v_data) {
    printf("Opening YUV420P file: %s (%dx%d)\n", input_file, width, height);

    *fp = fopen(input_file, "rb");
    if (!*fp) {
        fprintf(stderr, "Failed to open file: %s\n", input_file);
        return -1;
    }

    // 分配YUV数据缓冲区
    int y_size = width * height;
    int u_size = y_size / 4;
    int v_size = y_size / 4;

    *y_data = (unsigned char *)malloc(y_size);
    *u_data = (unsigned char *)malloc(u_size);
    *v_data = (unsigned char *)malloc(v_size);

    if (!*y_data || !*u_data || !*v_data) {
        fprintf(stderr, "Failed to allocate memory\n");
        if (*y_data) free(*y_data);
        if (*u_data) free(*u_data);
        if (*v_data) free(*v_data);
        fclose(*fp);
        return -1;
    }

    return 0;
}

/**
 * 关闭文件并释放内存
 */
void close_yuv_file(FILE *fp, unsigned char *y_data, 
                    unsigned char *u_data, unsigned char *v_data) {
    if (y_data) free(y_data);
    if (u_data) free(u_data);
    if (v_data) free(v_data);
    if (fp) fclose(fp);
}

int main(int argc, char *argv[]) {
    const char *input_file = "input.yuv";
    int width = 1920;
    int height = 1080;
    
    // 可以从命令行参数获取文件名和分辨率
    if (argc >= 4) {
        input_file = argv[1];
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    }

    FILE *fp = NULL;
    unsigned char *y_data = NULL;
    unsigned char *u_data = NULL;
    unsigned char *v_data = NULL;

    // 打开文件并分配内存
    if (open_yuv_file(input_file, width, height, &fp, &y_data, &u_data, &v_data) != 0) {
        return -1;
    }

    int frame_count = 0;
    
    // 循环读取每一帧
    while (1) {
        int ret = read_yuv420p_frame(fp, width, height, y_data, u_data, v_data);
        if (ret != 0) {
            if (feof(fp)) {
                printf("Reached end of file\n");
            } else {
                fprintf(stderr, "Error reading frame %d\n", frame_count);
            }
            break;
        }

        frame_count++;
        printf("Successfully read frame %d\n", frame_count);

        // 在这里可以对读取的YUV数据进行处理
        // 例如：编码、显示等
    }

    printf("Total frames read: %d\n", frame_count);

    // 关闭文件并释放资源
    close_yuv_file(fp, y_data, u_data, v_data);

    return 0;
}
