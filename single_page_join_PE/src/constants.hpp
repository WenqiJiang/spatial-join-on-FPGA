#pragma once

#define OBJ_BYTES 20 // 20 bytes (1 * id + 4 * boundary)
#define OBJ_BITS (OBJ_BYTES * 8) // 20 bytes (1 * id + 4 * boundary)
#define N_OBJ_PER_AXI 3 // 64 bytes can accommodate 3 objects 