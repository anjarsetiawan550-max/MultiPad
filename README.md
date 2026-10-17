# MULTI PAD LR PRO  (VST3 + Standalone)

Sampler 6 pad bertema Doraemon biru, dibuat dengan JUCE 7. GUI memakai asset kit
`MultiPad_LR_PRO_Doraemon_GUI_Assets.zip` (sudah di-embed di folder `Assets/`).

## Fitur
- **6 pad**: klik kiri mouse **atau** tombol keyboard **1-6** (baris angka & numpad).
- **Tombol tetap bunyi walau window plugin tidak aktif** (Windows & macOS).
  Matikan/hidupkan lewat klik kanan di area luar pad. Kombinasi dengan Ctrl/Alt/Win/Cmd diabaikan.
- **MIDI**: note C1-F1 (36-41) memainkan PAD 1-6 (berguna saat dipakai sebagai instrumen di DAW).
- **Klik kanan setiap pad**:
  - **Upload Sampling...** - pilih file audio dari penyimpanan (WAV, AIFF, FLAC, OGG, MP3, dan M4A/AAC/WMA bila didukung OS).
  - **Setting...** - Durasi 1-10 detik, Feed In 0-5 detik, Feed Out 0-5 detik (0 = Off), Volume 1-100.
  - **Rename...** - ganti nama pad (maks. 12 karakter). Ukuran teks otomatis menyesuaikan lebar pad.
  - Hapus Sampling.
- Klik kiri pada pad yang masih kosong langsung membuka dialog upload.
- Sampel & pengaturan **tersimpan di dalam project DAW** (data audio ikut tersimpan, tidak bergantung lokasi file).
- Setting tiap pad juga muncul sebagai parameter (bisa di-automate di DAW).
- Window bisa di-resize (rasio 1:1).

Catatan: hanya 10 detik pertama dari file yang dibaca, sesuai batas durasi maksimum.

## Build lewat GitHub Actions
1. Buat repo baru di GitHub, upload **seluruh isi folder ini** (termasuk folder `.github` dan `Assets`).
2. Buka tab **Actions**, workflow *Build MULTI PAD LR PRO* berjalan otomatis
   (atau klik **Run workflow**).
3. Setelah hijau, unduh hasil di bagian **Artifacts**:
   `MULTI_PAD_LR_PRO_Windows` dan `MULTI_PAD_LR_PRO_macOS`.
4. (Opsional) Buat tag `v1.0.0` untuk otomatis membuat halaman Release berisi file zip.

## Instalasi
- **Windows**: salin folder `MULTI PAD LR PRO.vst3` ke `C:\Program Files\Common Files\VST3`.
  Aplikasi standalone: `Standalone\MULTI PAD LR PRO.exe`.
- **macOS**: salin `MULTI PAD LR PRO.vst3` ke `~/Library/Audio/Plug-Ins/VST3`.
  Build tidak ber-notarize; jika diblokir, klik kanan > Open, atau jalankan
  `xattr -dr com.apple.quarantine "<file>"`. Untuk tombol global, izinkan aplikasi di
  *System Settings > Privacy & Security > Input Monitoring* bila diminta.

## Build lokal (opsional)
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Hasil ada di `build/MultiPadLR_artefacts/Release/`.

## Struktur
```
CMakeLists.txt
Assets/            bg.png, pad1-6.png (tanpa label), icon.png
Source/            PluginProcessor, PluginEditor, SettingsPanel, LRLookAndFeel, GlobalKeys
.github/workflows/build.yml
```
