#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

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

/**
 * 这个函数现在由EncodeContextWriteYuvData替代
 */

int main(int argc, char *argv[]) {
    // 编码前100帧
    const char *input_file = "test.yuv";
    const char *output_file = "output.h265";
    int width = 3840;
    int height = 2160;
    int max_frames = 100; // 编码前100帧
    
    printf("=== Intel Hardware HEVC Encoder ===\n");
    printf("输入文件: %s\n", input_file);
    printf("输出文件: %s\n", output_file);
    printf("分辨率: %dx%d\n", width, height);
    printf("最大帧数: %d\n", max_frames);
    
    FILE *fp = NULL;
    unsigned char *y_data = NULL;
    unsigned char *u_data = NULL;
    unsigned char *v_data = NULL;

    // 1. 打开YUV文件并分配内存
    printf("\n1. 打开YUV文件并分配内存...\n");
    if (open_yuv_file(input_file, width, height, &fp, &y_data, &u_data, &v_data) != 0) {
        return -1;
    }
    printf("YUV文件打开成功\n");

    // 2. 创建GPU上下文
    printf("\n2. 创建GPU上下文...\n");
    struct GpuContext* gpu_context = GpuContextCreate(kItuRec709, kFullRange);
    if (!gpu_context) {
        fprintf(stderr, "Failed to create GPU context\n");
        close_yuv_file(fp, y_data, u_data, v_data);
        return -1;
    }
    printf("GPU上下文创建成功\n");

    // 3. 创建编码上下文
    printf("\n3. 创建编码上下文...\n");
    struct EncodeContext* encode_context = EncodeContextCreate(
        gpu_context, width, height, kItuRec709, kFullRange);
    if (!encode_context) {
        fprintf(stderr, "Failed to create encode context\n");
        GpuContextDestroy(gpu_context);
        close_yuv_file(fp, y_data, u_data, v_data);
        return -1;
    }
    printf("编码上下文创建成功\n");

    // 4. 获取编码器输入帧
    printf("\n4. 获取编码器输入帧...\n");
    const struct GpuFrame* encoded_frame = EncodeContextGetFrame(encode_context);
    if (!encoded_frame) {
        fprintf(stderr, "Failed to get encoder input frame\n");
        EncodeContextDestroy(encode_context);
        GpuContextDestroy(gpu_context);
        close_yuv_file(fp, y_data, u_data, v_data);
        return -1;
    }
    printf("编码器输入帧获取成功 (分辨率: %dx%d)\n", 
           encoded_frame->width, encoded_frame->height);

    // 5. 创建输出文件
    printf("\n5. 创建输出文件...\n");
    int output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (output_fd == -1) {
        fprintf(stderr, "Failed to create output file: %s\n", strerror(errno));
        EncodeContextDestroy(encode_context);
        GpuContextDestroy(gpu_context);
        close_yuv_file(fp, y_data, u_data, v_data);
        return -1;
    }
    printf("输出文件创建成功\n");

    // 6. 开始编码过程 - 编码100帧
    printf("\n6. 开始编码YUV帧 (目标: %d帧)...\n", max_frames);
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    int encoded_frames = 0;
    int keyframes = 0;
    int failed_frames = 0;
    
    for (int frame_num = 0; frame_num < max_frames; frame_num++) {
        // 进度指示
        if (frame_num % 10 == 0) {
            printf("\n=== 进度: %d/%d (%.1f%%) ===\n", frame_num + 1, max_frames, 
                   (float)(frame_num + 1) / max_frames * 100);
        }
        
        printf("编码帧 %d/%d... ", frame_num + 1, max_frames);
        
        // 从文件读取YUV帧
        int ret = read_yuv420p_frame(fp, width, height, y_data, u_data, v_data);
        if (ret != 0) {
            if (feof(fp)) {
                printf("\n📄 已到达文件结尾 (共读取%d帧)\n", frame_num);
                break;
            } else {
                fprintf(stderr, "\n❌ 第%d帧：读取YUV数据失败\n", frame_num + 1);
                failed_frames++;
                continue;
            }
        }
        
        // 直接将YUV数据写入编码器表面
        printf("写入... ");
        if (!EncodeContextWriteYuvData(encode_context, y_data, u_data, v_data, width, height)) {
            fprintf(stderr, "❌ 写入失败\n");
            failed_frames++;
            continue;
        }
        printf("✓ ");
        
        // 获取时间戳（微秒级别）
        unsigned long long timestamp;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        timestamp = (unsigned long long)tv.tv_sec * 1000000ULL + tv.tv_usec;
        
        // 编码帧
        printf("编码... ");
        bool is_keyframe = (frame_num % 30 == 0); // 每30帧一个关键帧
        bool success = EncodeContextEncodeFrame(encode_context, output_fd, timestamp);
        
        if (success) {
            encoded_frames++;
            if (is_keyframe) keyframes++;
            printf("✅");
            if (is_keyframe) printf(" 🔑关键帧");
            
            // 每10帧显示进度统计
            if ((frame_num + 1) % 10 == 0) {
                printf(" [已完成%d帧, 成功率:%.1f%%]", 
                       encoded_frames, (float)encoded_frames / (frame_num + 1) * 100);
            }
            printf("\n");
        } else {
            fprintf(stderr, "❌ 编码失败\n");
            failed_frames++;
            // 继续尝试下一帧，不要立即退出
            continue;
        }
        
        // 小延迟以模拟真实场景
        usleep(1000); // 1ms
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // 计算性能统计
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                         (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    double fps = encoded_frames > 0 ? encoded_frames / elapsed_time : 0;
    
    // 关闭输出文件
    close(output_fd);
    
    // 输出测试结果
    printf("\n" "═══════════════════════════════════════════════════════════════\n");
    printf("🎬 编码完成统计\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("📊 编码结果:\n");
    printf("  • 目标帧数: %d\n", max_frames);
    printf("  • 成功编码: %d 帧\n", encoded_frames);
    printf("  • 失败帧数: %d 帧\n", failed_frames);
    printf("  • 关键帧数: %d 帧\n", keyframes);
    printf("  • 成功率: %.2f%%\n", (float)encoded_frames / max_frames * 100);
    
    printf("\n⏱️  性能统计:\n");
    printf("  • 总耗时: %.3f 秒\n", elapsed_time);
    if (encoded_frames > 0) {
        printf("  • 编码速度: %.2f FPS\n", fps);
        printf("  • 平均帧延迟: %.2f 毫秒\n", (elapsed_time / encoded_frames) * 1000);
    }
    
    // 检查输出文件大小
    struct stat st;
    if (stat(output_file, &st) == 0) {
        printf("\n📁 输出文件信息:\n");
        printf("  • 文件路径: %s\n", output_file);
        printf("  • 文件大小: %.2f MB (%ld 字节)\n", st.st_size / (1024.0 * 1024.0), st.st_size);
        if (st.st_size > 0 && encoded_frames > 0) {
            double bitrate_mbps = (st.st_size * 8.0) / (elapsed_time * 1000000);
            printf("  • 平均码率: %.2f Mbps\n", bitrate_mbps);
            printf("  • 每帧平均大小: %.2f KB\n", st.st_size / (1024.0 * encoded_frames));
        }
    }
    
    // 清理资源
    printf("\n7. 清理资源...\n");
    EncodeContextDestroy(encode_context);
    GpuContextDestroy(gpu_context);
    close_yuv_file(fp, y_data, u_data, v_data);
    
    printf("\n=== 编码完成 ===\n");
    
    if (encoded_frames > 0) {
        printf("✅ 编码成功！输出文件: %s\n", output_file);
        return 0;
    } else {
        printf("❌ 编码失败：没有成功编码任何帧\n");
        return 1;
    }
}
