#ifndef NISKALA_TARGET_STORE_H
#define NISKALA_TARGET_STORE_H

#include <Arduino.h>
#include <stdint.h>

#define TARGET_MAX 20

struct SavedTarget {
  uint8_t mac[6];
  char name[33];
  uint8_t channel;
  bool used;
};

extern SavedTarget g_savedTargets[TARGET_MAX];
extern int g_savedTargetCount;

// Muat daftar target tersimpan dari NVS (idempotent).
void targetStoreLoad();

// Simpan seluruh daftar ke NVS.
void targetStoreSave();

// Tambah/update target; return false jika penuh atau argumen tidak valid.
bool targetAdd(const char* name, const uint8_t mac[6], uint8_t channel);

// Hapus target berdasarkan index (0-based).
bool targetDelete(int idx);

// Cari target berdasarkan nama (case-sensitive). Return true jika ditemukan.
bool targetFindByName(const char* name, SavedTarget* out);

// Cetak daftar target ke Serial.
void targetList();

#endif