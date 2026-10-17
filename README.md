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

## Build Windows lewat GitHub Actions
1. Upload **seluruh isi folder proyek** ke repository GitHub, termasuk folder `.github` dan `Assets`.
2. Buka tab **Actions**. Workflow *Build MULTI PAD LR PRO (Windows)* berjalan saat push/PR, atau jalankan lewat **Run workflow**.
3. Setelah job **Build (Windows)** berstatus hijau, unduh artifact `MULTI_PAD_LR_PRO_Windows`. ZIP berisi plugin VST3 dan aplikasi Standalone untuk Windows x64.
4. (Opsional) Buat tag `v1.0.0` untuk menerbitkan Windows ZIP pada halaman Releases.

## Instalasi
- **Windows**: salin folder `MULTI PAD LR PRO.vst3` ke `C:\Program Files\Common Files\VST3`.
  Aplikasi standalone: `Standalone\MULTI PAD LR PRO.exe`.

## Build Windows lokal (opsional)
Jalankan dari Developer PowerShell/terminal yang memiliki CMake dan Visual Studio C++ Build Tools:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
```
Hasil ada di `build/MultiPadLR_artefacts/Release/`. Workflow CI saat ini hanya menghasilkan paket Windows x64.

## Struktur
```
CMakeLists.txt
Assets/            bg.png, pad1-6.png (tanpa label), icon.png
Source/            PluginProcessor, PluginEditor, SettingsPanel, LRLookAndFeel, GlobalKeys
.github/workflows/main.yml
```
