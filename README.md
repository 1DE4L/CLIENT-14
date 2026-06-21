# CLIENT 14

DDNet tabanlı özel antrenman istemcisi. Bot/yardımcı özellikler içerir.

## Ozellikler

- **Aimbot**: FOV tabanli otomatik nisan, silent aim, target onceligi
- **Auto Hook**: Menzildeki hedefe otomatik kanca (tusu basili tuttukca)
- **Anti-Freeze**: Donunca en yakin oyuncuya otomatik hook
- **Avoid Freeze**: Freeze tile'larini tahmine dayali fizik simulyasyonuyla sagma
- **Auto Jump Save**: Freeze tile'a dusmek uzereyken otomatik ziplama
- **Hammer Bot**: Yakindaki dusmanlara otomatik hammer
- **Freeze Unfreeze**: Donmus takim arkadaslarini hammer ile cozdur
- **Auto Fire**: Tutulu ates
- **Quick Stop**: Hizli fren (zincir hook'lara duyarli)
- **Balance Bot**: En yakin oyuncuya yatay hizalama (PID + hysteresis)
- **Chat Translator**: Gelen/giden chat'i otomatik cevir (mymemory API)
- **Warlist / Player Tracker / Chat Balloons / HUD overlay**

## Arkadaslar Icin Kurulum (Kaynaktan Derleme)

### Gereksinimler (Windows)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) (C++ workload ile)
- [CMake](https://cmake.org/)
- [Python 3](https://www.python.org/)
- [Rust](https://rustup.rs/)

### Adimlar
```powershell
# 1. Repoyu klonla
git clone https://github.com/USER/CLIENT14.git
cd CLIENT14/ddnet

# 2. Dis kutuphaneleri klonla (build icin sart, ~400MB)
git clone --depth 1 --shallow-submodules https://github.com/ddnet/ddnet-libs ddnet-libs

# 3. Derleme
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target game-client

# 4. Calistir
build\release\CLIENT14.exe
```

### Hazir EXE (Derleme Yapmadan)

Release ZIP'ini indir, klasore cikart ve `CLIENT14.exe`'yi calistir.

## Bot Ayarlari

Oyun icinde **Settings → CLIENT 14** sayfasindan tum botlar acilip kapatilabilir.
Hotkeys ayarlari ayni sayfada.

## Uyari

Bu istemci bot/yardimci ozellikler icerir. DDNet resmi sunucularinda bot
kullanimi banlanabilir. Yerel/test sunucularinda kullanin.

## Lisans

DDNet upstream lisansi: bkz. `ddnet/license.txt`
