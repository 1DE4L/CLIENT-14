# DDNet CLIENT 14 Özel Client Geliştirme Yol Haritası

Bu plan, en güncel resmi DDNet kod tabanını kullanarak CLIENT 14 isimli özel antrenman istemcisini (train client) kurma, derleme ve özelleştirme adımlarını içerir.

## Planlanan Özellikler (CLIENT 14)

### 1. Temel Arayüz ve Markalama
- İstemci ismi her yerde CLIENT 14 olarak güncellenecek.
- Arayüz renk paleti, yükleme ekranları ve yazı tipleri özelleştirilecek.

### 2. Genel Yaşam Kalitesi (QoL) ve Görsel Özellikler
- **BindWheel**: Kolay bind yönetimi için dairesel menü.
- **Warlist**: Oyuncu gruplarını yönetme (renkli isimler vb.).
- **Player Tracker & HUD**: Ekrandaki ve ekran dışındaki oyuncuları takip eden göstergeler.
- **Görsel İyileştirmeler**: Renkli lazerler, chat balonları, özel yazı tipleri.

### 3. Antrenman & Yardımcı Özellikler (Train Features)
- **Avoid Freeze (Anti-Freeze)**: Haritadaki dondurucu (freeze) bölgelerinden kaçınmaya yardımcı olacak veya buralarda hareketi optimize edecek mekanizma / uyarılar.
- **Aimbot / Hookbot**: Kanca (hook) atarken hedefleri kolayca vurmak için otomatik nişan alma veya kanca tahmin yardımcısı.

---

## Önerilen Değişiklikler ve Adımlar

### Adım 1: Geliştirme Ortamının Kurulması
Sisteminizde gerekli derleme araçlarının bulunmadığı tespit edilmiştir. Windows Paket Yöneticisi (winget) kullanılarak kurulumlar aşağıdaki gibi gerçekleştirilecektir:

**Git Kurulumu:**
```powershell
winget install Git.Git --silent
```

**Python 3.12 Kurulumu:**
```powershell
winget install Python.Python.3.12 --silent
```

**CMake Kurulumu:**
```powershell
winget install Kitware.CMake --silent
```

**Rust Kurulumu (Rustup):**
```powershell
winget install Rustlang.Rustup --silent
```

**Visual Studio 2022 Community Kurulumu (C++ Geliştirme Araçları ile):**
```powershell
winget install Microsoft.VisualStudio.2022.Community --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive --norestart"
```

> **Not:** Kurulumların ardından sistem ortam değişkenlerinin (PATH) güncellenmesi için terminalin/bilgisayarın yeniden başlatılması gerekebilir.

### Adım 2: DDNet Deposunun Klonlanması
Kurulumlar tamamlandıktan sonra, en güncel resmi DDNet deposu alt modülleriyle birlikte klonlanacaktır:

```powershell
git clone --recursive https://github.com/ddnet/ddnet "c:\Users\altne\Desktop\CLIENT 14\ddnet"
```

### Adım 3: Derleme Testi ve Markalama
İstemcinin başarıyla derlendiğini doğrulamak için:

```powershell
cd "c:\Users\altne\Desktop\CLIENT 14\ddnet"
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target game-client
```

Derleme sonrası arayüzde CLIENT 14 markalaması yapılacak ve ilk testler gerçekleştirilecektir.

---

## Doğrulama Planı

### Otomatik Testler & Derleme Kontrolü
Her yeni özellik veya modül eklendiğinde projenin derlenebilirliği test edilecektir.

### Manuel Doğrulama
- Derlenen istemciyi başlatıp ana menüdeki sürüm isminin CLIENT 14 olduğunu kontrol etmek.
- Yerel bir sunucuda aimbot ve avoid freeze modüllerinin işlevselliğini test etmek.
