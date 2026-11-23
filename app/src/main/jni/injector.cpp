#include <jni.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <dirent.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FFMX", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFMX", __VA_ARGS__)

struct Vec3 { float x, y, z; };

namespace offsets {
    uintptr_t IL2CPP_BASE = 0x0;
    // ob55 অনুমানভিত্তিক অফসেট (গেম আপডেটের ওপর নির্ভরশীল)
    uintptr_t ENTITY_LIST = 0x9C8E14C;    // Example Entity List
    uintptr_t ENTITY_STRIDE = 0x4;        // Player Size
    uintptr_t VIEW_MATRIX = 0x9A2B8F0;    // Example View Matrix
    uintptr_t LOCAL_PLAYER = 0x9C8E150;   // Example Local Player
    uintptr_t HEAD_BONE = 0x3A4;          // Example Head Bone Offset
    uintptr_t TEAM_OFFSET = 0x2B0;        // Example Team Offset
    uintptr_t HEALTH_OFFSET = 0x2A8;      // Example Health Offset
}

template <typename T>
T ReadMemory(uintptr_t addr) {
    if (addr == 0) return T{};
    return *(T*)addr;
}

bool WorldToScreen(const Vec3& w, const float* m, int sw, int sh, float& sx, float& sy) {
    float cx = w.x*m[0] + w.y*m[4] + w.z*m[8]  + m[12];
    float cy = w.x*m[1] + w.y*m[5] + w.z*m[9]  + m[13];
    float cw = w.x*m[3] + w.y*m[7] + w.z*m[11] + m[15];
    if (cw < 0.1f) return false;
    sx = (sw * 0.5f) * (1.0f + cx / cw);
    sy = (sh * 0.5f) * (1.0f - cy / cw);
    return true;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_panel_ffmax_MainActivity_inject(JNIEnv* env, jobject thiz, jstring pkg) {
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    
    // 1. Get libil2cpp.so base
    FILE* file = fopen("/proc/self/maps", "r");
    char line[512];
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "libil2cpp.so")) {
                offsets::IL2CPP_BASE = strtoull(line, nullptr, 16);
                break;
            }
        }
        fclose(file);
    }

    if (offsets::IL2CPP_BASE == 0) {
        LOGE("libil2cpp.so not found! Game must be running.");
        return -1;
    }

    LOGI("Engine started. Base: 0x%lx", offsets::IL2CPP_BASE);
    
    // 2. Main Loop (ESP & Aimbot logic)
    while (true) {
        if (offsets::ENTITY_LIST != 0 && offsets::VIEW_MATRIX != 0) {
            uintptr_t entityList = ReadMemory<uintptr_t>(offsets::IL2CPP_BASE + offsets::ENTITY_LIST);
            
            if (entityList != 0) {
                float viewMatrix[16];
                memcpy(viewMatrix, (void*)(offsets::IL2CPP_BASE + offsets::VIEW_MATRIX), sizeof(viewMatrix));

                // Loop through 50 entities
                for (int i = 0; i < 50; i++) {
                    uintptr_t entity = entityList + (i * offsets::ENTITY_STRIDE);
                    int health = ReadMemory<int>(entity + offsets::HEALTH_OFFSET);
                    int team = ReadMemory<int>(entity + offsets::TEAM_OFFSET);
                    
                    if (health > 0 && health < 1000 && team != ReadMemory<int>(ReadMemory<uintptr_t>(offsets::IL2CPP_BASE + offsets::LOCAL_PLAYER) + offsets::TEAM_OFFSET)) {
                        Vec3 pos = ReadMemory<Vec3>(entity + offsets::HEAD_BONE);
                        float sx, sy;
                        if (WorldToScreen(pos, viewMatrix, 1080, 2400, sx, sy)) {
                            // Here you would call your ESP drawing function
                            // e.g., DrawBox(sx, sy, health);
                            
                            // Auto-aim logic
                            // e.g., MoveCrosshair(sx, sy);
                            LOGI("Enemy detected at screen: %f, %f", sx, sy);
                        }
                    }
                }
            }
        }
        usleep(10000); // 10ms delay
    }
    
    env->ReleaseStringUTFChars(pkg, p);
    return 0;
}
