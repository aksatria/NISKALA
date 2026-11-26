# Unit Test NISKALA

Jalankan:

```bash
pio test -e native
```

Test saat ini berjalan di host (native), cocok untuk logika murni yang
tidak bergantung pada Arduino/ESP32. Test on-device bisa ditambahkan di
`platformio.ini` environment embedded saat hardware tersedia.