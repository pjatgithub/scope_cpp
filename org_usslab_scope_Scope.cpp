//
// Created by Administrator on 2017/2/14.
//
#include "org_usslab_scope_Scope.h"
#include <limits>

HINSTANCE m_dll = nullptr;
//Handles to functions in visa32.dll
handle_viOpenDefaultRM m_viOpenDefaultRM = nullptr;
handle_viOpen m_viOpen = nullptr;
handle_viClose m_viClose = nullptr;
handle_viPrintf m_viPrintf = nullptr;
handle_viRead m_viRead = nullptr;
ViSession m_defaultRM;
int m_status = SCOPE_NOT_INITIALIZED;

JNIEXPORT jint JNICALL Java_org_usslab_scope_Scope_initialize
        (JNIEnv *, jobject) {
    if(m_status == SCOPE_NOT_INITIALIZED) {
        // Find functions in C:/Windows/System32/visa32.dll
        if ((m_dll = LoadLibrary(DLL_NAME)) == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_DLL_NOT_FOUND;
        }
        // Find "viOpenDefaultRM" function in dll
        if ((m_viOpenDefaultRM = (handle_viOpenDefaultRM) GetProcAddress(m_dll, FUN_NAME_VI_OPEN_DEFAULT_RM))
            == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_FUN_NOT_FOUND;
        }
        // Find "viOpen" function in dll
        if ((m_viOpen = (handle_viOpen) GetProcAddress(m_dll, FUN_NAME_VI_OPEN)) == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_FUN_NOT_FOUND;
        }
        // Find "viClose" function in dll
        if ((m_viClose = (handle_viClose) GetProcAddress(m_dll, FUN_NAME_VI_CLOSE)) == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_FUN_NOT_FOUND;
        }
        // Find "viPrintf" function in dll
        if ((m_viPrintf = (handle_viPrintf) GetProcAddress(m_dll, FUN_NAME_VI_PRINTF)) == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_FUN_NOT_FOUND;
        }
        // Find "viRead" function in dll
        if ((m_viRead = (handle_viRead) GetProcAddress(m_dll, FUN_NAME_VI_READ)) == nullptr) {
            FreeLibrary(m_dll);
            return SCOPE_ERR_FUN_NOT_FOUND;
        }
        // Open default resource manager
        if(m_viOpenDefaultRM(&m_defaultRM) != VI_SUCCESS) {
            m_viClose(m_defaultRM);
            FreeLibrary(m_dll);
            return SCOPE_ERR_CONNECTION;
        }
        // Succeed
        m_status = SCOPE_INITIALIZED;
        return SCOPE_SUCCESS;
    } else
        return SCOPE_SUCCESS;
}

JNIEXPORT jlong JNICALL Java_org_usslab_scope_Scope_decode
        (JNIEnv *, jobject) {
    // Local variables
    ViSession session;
    uint8_t buffer[BUFFER_SIZE];
    ViUInt32 count = 0;
    int64_t acceleration;

    // Connect scope
    if(m_status == SCOPE_INITIALIZED) {
        if (m_viOpen(m_defaultRM, (ViRsrc) SCOPE_CONNECTION_STRING_USB, VI_NULL, SCOPE_TIMEOUT_MS, &session)
            == VI_SUCCESS) {
            m_viPrintf(session, (ViString) "run\n");
            m_viPrintf(session, (ViString) ":waveform:mode norm\n");
            m_viPrintf(session, (ViString) ":waveform:format byte\n");
            m_viPrintf(session, (ViString) ":waveform:source chan2\n");
            // Attain data
            do {
                m_viPrintf(session, (ViString) ":waveform:data?\n");
                m_viRead(session, buffer, BUFFER_SIZE, &count);
                if(count != BYTE_READ) {
                    acceleration = CODE_NULL;
                    continue;
                }
                acceleration = decode(buffer, HEADER_BIT_SIZE, DATA_SIZE);
            } while (acceleration == CODE_NULL);
        } else
            acceleration = CODE_NULL;

        m_viClose(session);
        return acceleration;
    } else
        return CODE_NULL;
}

JNIEXPORT void JNICALL Java_org_usslab_scope_Scope_close
        (JNIEnv *, jobject) {
    if (m_status == SCOPE_NOT_INITIALIZED)
        return;
    else {
        m_viClose(m_defaultRM);
        FreeLibrary(m_dll);
        m_status = SCOPE_NOT_INITIALIZED;
        return;
    }
}

int64_t decode(const uint8_t *const data, const int start, const int size) {
    uint8_t min = std::numeric_limits<uint8_t>::max();
    uint8_t max = std::numeric_limits<uint8_t>::min();
    uint8_t middle;
    uint64_t code = 0ULL;
//    int min_index = start, max_index = start;
    int start_index;
    // Find min nad max
    for (int i = start; i < start + size; ++i) {
        if(data[i] < min) {
            min = data[i];
//            min_index = i;
        }
        if(data[i] > max) {
            max = data[i];
//            max_index = i;
        }
    }
    // Get code
    middle = (uint8_t) (((uint16_t)max + (uint16_t)min) / 2);
//    start_index = ((min_index - start) % DATA_STEP) + start;
    start_index = start + DATA_STEP / 2;
    for (int i = start_index; i < start + size; i += DATA_STEP) {
        code <<= 1;
        if(data[i] > middle) {
            code |= 1;
        }
    }
    // Verify code
//    return code;
    if((code & CODE_T1) != CODE_T1_VALUE)
        return CODE_NULL;
    if((code & CODE_T2) == CODE_T2_VALUE) {
        code = (code & CODE_T2_SELECTOR) >> CODE_T2_SHIFT;
        return de_manchester(code, CODE_BITS, 0);
    }
    if((code & CODE_T3) == CODE_T3_VALUE) {
        code = (code & CODE_T3_SELECTOR) >> CODE_T3_SHIFT;
        return de_manchester(code, CODE_BITS, 0);
    }
    if((code & CODE_T4) == CODE_T4_VALUE) {
        code = (code & CODE_T4_SELECTOR) >> CODE_T4_SHIFT;
        return de_manchester(code, CODE_BITS, 0);
    }
    if((code & CODE_T5) == CODE_T5_VALUE) {
        code = (code & CODE_T5_SELECTOR) >> CODE_T5_SHIFT;
        return de_manchester(code, CODE_BITS, 0);
    }
    if((code & CODE_T6) == CODE_T6_VALUE) {
        code = (code & CODE_T6_SELECTOR) >> CODE_T6_SHIFT;
        return de_manchester(code, CODE_BITS, 0);
    }
    return CODE_NULL;
}

uint64_t de_manchester(uint64_t code, uint32_t size, bool check_mode) {
    uint64_t code_temp = 0ULL;
    uint64_t temp;
    bool check;

    // Acquire check bit
    temp = code & 0x3ULL;
    if(temp == 0x2ULL) // 0
        check = 0;
    else if(temp == 0x01ULL) // 1
        check = 1;
    else
        return CODE_NULL;

    for (int i = 0; i < size; ++i) {
        code >>= 2;
        temp = code & 0x3ULL;
        if(temp == 0x2ULL) { // 0
            code_temp <<= 1;
            code_temp |= 0ULL;
            check ^= 0;
        }
        else if(temp == 0x01ULL) { // 1
            code_temp <<= 1;
            code_temp |= 1ULL;
            check ^= 1;
        }
        else
            return CODE_NULL;
    }

    // Check
    if(check != check_mode)
        return CODE_NULL;

    if((code_temp & (0x01ULL << size - 1)) == 0ULL)
        return code_temp;
    else
        return code_temp | (0xFFFFFFFFFFFFFFFFULL << size);
}