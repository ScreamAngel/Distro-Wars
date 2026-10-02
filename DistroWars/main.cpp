#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Oyun Durumları ve Modlar
enum OyunDurumu { 
    MENU_ANA, 
    MENU_MOD_SEC, 
    OYUN_AKTIF, 
    OYUN_YUKSELTME, 
    OYUN_BITTI, 
    OYUN_KAZANDIN 
};

enum OyunModu { 
    MOD_ENDLESS, 
    MOD_MINI_FIGHT 
};

enum DistroTipi {
    DISTRO_MINT,     
    DISTRO_DEBIAN,   
    DISTRO_FEDORA,   
    DISTRO_KALI,     
    DISTRO_POPOS,    
    DISTRO_ZORIN,    
    DISTRO_UBUNTU    
};

enum ItemTipi { ITEM_CAN, ITEM_MERMI, ITEM_ZIRH };

struct ItemDrop {
    float x, y;
    ItemTipi tip;
    int miktar;
    bool aktif;
};

struct ZeminDikeni {
    float x, y;
    float genislik, yukseklik;
    float kalanSure;
    bool aktif;
};

struct Dusman {
    float x, y;
    float hizY;
    float genislik, yukseklik;
    int can;
    int maxCan;
    float hiz;
    int yon; 
    float sinirSol, sinirSag; 
    float atesSayaci;
    DistroTipi distro;
    bool isBoss;
    bool hayattaMi;
    
    float ozelYetenekSayaci;
    int bossDurum;        
    float yetenekSayaci;  
    float durumSuresi;    
    bool bossParryAktif;
};

struct Mermi {
    float x, y;
    float hizX;
    bool aktif;
    bool yansitildi; 
};

struct Kart {
    const char* baslik;
    const char* aciklama;
    int id;
};

// --- CİLA: PARTİKÜL SİSTEMİ ---
struct Parcacik {
    float x, y;
    float hizX, hizY;
    float omur;
    float maxOmur;
    float boyut;
    Color renk;
    bool aktif;
};

// --- CİLA: UÇAN HASAR YAZILARI ---
struct UcanYazi {
    float x, y;
    char metin[24];
    Color renk;
    float omur;
    bool aktif;
};

// --- CİLA: GENİŞLEYEN ŞOK DALGASI ---
struct SokDalgasi {
    float x, y;
    float yaricap;
    float maxYaricap;
    float kalinlik;
    Color renk;
    bool aktif;
};

// Kalıcı Hafıza
void IstatistikleriYukle(int &tekrar, float &enIyi, int &enYuksekD) {
    FILE *dosya = fopen("save.txt", "r");
    if (dosya) {
        fscanf(dosya, "%d %f %d", &tekrar, &enIyi, &enYuksekD);
        fclose(dosya);
    } else {
        tekrar = 0;
        enIyi = -1.0f;
        enYuksekD = 0;
    }
}

void IstatistikleriKaydet(int tekrar, float enIyi, int enYuksekD) {
    FILE *dosya = fopen("save.txt", "w");
    if (dosya) {
        fprintf(dosya, "%d %.2f %d", tekrar, enIyi, enYuksekD);
        fclose(dosya);
    }
}

Texture2D GuvenliTextureYukle(const char* dosyaAdi, int w, int h) {
    if (FileExists(dosyaAdi)) {
        Image img = LoadImage(dosyaAdi);
        ImageResize(&img, w, h);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }
    return (Texture2D){ 0, w, h, 1, 7 };
}

// Güvenli Ses Çalma Fonksiyonu
void GuvenliSesCal(Sound s) {
    if (s.stream.buffer != NULL) {
        PlaySound(s);
    }
}

int main() {
    InitWindow(800, 600, "2D-Hikaye Oyunu - Distro Wars (Polished Edition)");
    SetTargetFPS(60);

    // --- SES MOTORUNU BAŞLAT ---
    InitAudioDevice();

    // Ses Efektleri (Varsa yükler, yoksa boş kalır - ASLA ÇÖKMEZ)
    Sound sndAtes   = FileExists("ates.wav")  ? LoadSound("ates.wav")  : (Sound){ 0 };
    Sound sndParry  = FileExists("parry.wav") ? LoadSound("parry.wav") : (Sound){ 0 };
    Sound sndHit    = FileExists("hit.wav")   ? LoadSound("hit.wav")   : (Sound){ 0 };
    Sound sndDash   = FileExists("dash.wav")  ? LoadSound("dash.wav")  : (Sound){ 0 };
    Sound sndSlam   = FileExists("slam.wav")  ? LoadSound("slam.wav")  : (Sound){ 0 };
    Sound sndLoot   = FileExists("loot.wav")  ? LoadSound("loot.wav")  : (Sound){ 0 };

    // Kaplamalar
    Image arkaplanImg = LoadImage("arka.gif");
    if (arkaplanImg.data != NULL) {
        ImageResize(&arkaplanImg, 800, 600);
    } else {
        arkaplanImg = GenImageColor(800, 600, BLACK);
    }
    Texture2D arkaplan = LoadTextureFromImage(arkaplanImg); 
    UnloadImage(arkaplanImg);

    Texture2D karakter = LoadTexture("archlinux.png");

    Texture2D texMint    = GuvenliTextureYukle("mint.png", 40, 40);
    Texture2D texDebian  = GuvenliTextureYukle("debian.png", 50, 50);
    Texture2D texFedora  = GuvenliTextureYukle("fedora.png", 40, 40);
    Texture2D texKali    = GuvenliTextureYukle("kali.png", 38, 38);
    Texture2D texPopos   = GuvenliTextureYukle("popos.png", 40, 40);
    Texture2D texZorin   = GuvenliTextureYukle("zorin.png", 40, 40);
    Texture2D texUbuntu  = GuvenliTextureYukle("ubuntu.png", 95, 95);

    // Platformlar
    Rectangle platformlar[] = {
        { 0, 500, 1600, 100 }, 
        { 300, 380, 150, 20 },  
        { 550, 280, 200, 20 },  
        { 850, 350, 150, 20 }   
    };
    int platformSayisi = 4;

    // --- EFEKT HAVUZLARI ---
    const int MAX_PARCACIK = 200;
    Parcacik parcaciklar[MAX_PARCACIK] = { 0 };

    const int MAX_YAZI = 35;
    UcanYazi ucanYazilar[MAX_YAZI] = { 0 };

    const int MAX_SOK = 10;
    SokDalgasi sokDalgalari[MAX_SOK] = { 0 };

    auto ParcacikPatlat = [&](float x, float y, Color renk, int adet, float maxHiz) {
        int eklendi = 0;
        for (int i = 0; i < MAX_PARCACIK && eklendi < adet; i++) {
            if (!parcaciklar[i].aktif) {
                parcaciklar[i].x = x;
                parcaciklar[i].y = y;
                float aci = (float)GetRandomValue(0, 360) * DEG2RAD;
                float hiz = (float)GetRandomValue(10, (int)(maxHiz * 10)) / 10.0f;
                parcaciklar[i].hizX = cosf(aci) * hiz;
                parcaciklar[i].hizY = sinf(aci) * hiz;
                parcaciklar[i].maxOmur = (float)GetRandomValue(25, 60) / 100.0f;
                parcaciklar[i].omur = parcaciklar[i].maxOmur;
                parcaciklar[i].boyut = (float)GetRandomValue(3, 6);
                parcaciklar[i].renk = renk;
                parcaciklar[i].aktif = true;
                eklendi++;
            }
        }
    };

    auto YaziEkle = [&](float x, float y, const char* txt, Color renk) {
        for (int i = 0; i < MAX_YAZI; i++) {
            if (!ucanYazilar[i].aktif) {
                ucanYazilar[i].x = x + GetRandomValue(-10, 10);
                ucanYazilar[i].y = y;
                strncpy(ucanYazilar[i].metin, txt, 23);
                ucanYazilar[i].renk = renk;
                ucanYazilar[i].omur = 0.85f;
                ucanYazilar[i].aktif = true;
                break;
            }
        }
    };

    auto SokDalgasiYarat = [&](float x, float y, float maxR, Color renk) {
        for (int i = 0; i < MAX_SOK; i++) {
            if (!sokDalgalari[i].aktif) {
                sokDalgalari[i].x = x;
                sokDalgalari[i].y = y;
                sokDalgalari[i].yaricap = 5.0f;
                sokDalgalari[i].maxYaricap = maxR;
                sokDalgalari[i].kalinlik = 4.0f;
                sokDalgalari[i].renk = renk;
                sokDalgalari[i].aktif = true;
                break;
            }
        }
    };

    // Kalıcı Veriler
    int miniFightTekrar = 0;
    float enIyiSure = -1.0f;
    int enYuksekDalga = 0;
    IstatistikleriYukle(miniFightTekrar, enIyiSure, enYuksekDalga);

    float kosuSuresi = 0.0f;
    bool zaferObjesiAktif = false;
    Rectangle zaferObjesi = { 0, 0, 30, 30 };

    OyunDurumu durum = MENU_ANA;
    OyunModu seciliMod = MOD_ENDLESS;
    int anaMenuSecim = 0;   
    int modMenuSecim = 0;   
    int kartSecim = 0;

    const int MAX_OYUNCU_MERMI = 60;
    Mermi oyuncuMermiler[MAX_OYUNCU_MERMI] = { 0 };

    const int MAX_DUSMAN_MERMI = 70;
    Mermi dusmanMermiler[MAX_DUSMAN_MERMI] = { 0 };

    const int MAX_DIKEN = 20;
    ZeminDikeni dikenler[MAX_DIKEN] = { 0 };

    int sarjorMermi = 12;
    int sarjorKapasite = 12;
    int yedekMermi = 60;
    bool reloadYapiyor = false;
    float reloadTimer = 0.0f;
    float reloadSuresi = 1.2f;
    int oyuncuMermiHasari = 25;
    int oyuncuYumrukHasari = 35;
    int oyuncuSlamHasari = 60;

    const int MAX_DUSMAN = 15;
    Dusman dusmanlar[MAX_DUSMAN];
    int dalga = 1;
    int dalgaDusmanSayisi = 3;
    int hayattakiDusman = 0;
    float dalgaArasiSayac = 0.0f;
    bool yeniDalgaBekleniyor = false;

    int bossFaz = 1;
    float faz2BildirimSayaci = 0.0f;

    const int MAX_ITEMS = 30;
    ItemDrop itemler[MAX_ITEMS] = { 0 };

    float karakterX = 100.0f; 
    float karakterY = 400.0f; 
    int karakterGenislik = 40;  
    int karakterYukseklik = 40;
    float hiz = 5.0f;         
    int karakterYon = 1;      
    
    float hizY = 0.0f;          
    float yercekimi = 0.5f;     
    float ziplamaGucu = -11.0f; 
    bool yerdeMi = false;  

    int maxCan = 100;
    int mevcutCan = 100;
    int maxZirh = 100;
    int mevcutZirh = 50;

    int maxZiplamaHakki = 5;
    int kalanHak = 5;
    float staminaTimer = 0.0f;       
    float staminaDolmaSuresi = 1.7f; 

    int maxDashHakki = 3;
    int kalanDash = 3;
    float dashTimer = 0.0f;
    float dashDolmaSuresi = 1.5f;   
    bool dashAtiyorMu = false;
    float dashKalanSure = 0.0f;
    float dashSuresi = 0.15f;       
    float dashHizi = 18.0f;         
    int dashYonu = 1;
    
    bool slamYapiyorMusu = false;
    float sarsintiSuresi = 0.0f; 
    float yumrukEfektSuresi = 0.0f;

    bool parryAktif = false;
    float parrySayaci = 0.0f;
    float parryBeklemeSuresi = 0.0f;
    float beyazFlashSuresi = 0.0f;

    Kart sunulanKartlar[3];

    int kapsulGenislik = 32;
    int kapsulYukseklik = 10;
    int bosluk = 6;
    int baslangicX = 20;

    Camera2D kamera = { 0 };
    kamera.offset = (Vector2){ 400.0f, 300.0f }; 
    kamera.rotation = 0.0f;
    kamera.zoom = 1.0f;

    auto DikenCikar = [&](float baslaX, float baslaY, int adet) {
        int eklendi = 0;
        for (int i = 0; i < MAX_DIKEN && eklendi < adet; i++) {
            if (!dikenler[i].aktif) {
                float ofset = (eklendi % 2 == 0) ? (eklendi * 40.0f) : -(eklendi * 40.0f);
                dikenler[i].x = baslaX + ofset;
                dikenler[i].y = baslaY;
                dikenler[i].genislik = 25.0f;
                dikenler[i].yukseklik = 32.0f;
                dikenler[i].kalanSure = 2.2f;
                dikenler[i].aktif = true;
                eklendi++;
            }
        }
    };

    auto DalgaUret = [&](int dalgaNo, OyunModu mod) {
        bossFaz = 1;
        faz2BildirimSayaci = 0.0f;

        if (mod == MOD_MINI_FIGHT && dalgaNo == 2) {
            dalgaDusmanSayisi = 1;
            hayattakiDusman = 1;

            dusmanlar[0].distro = DISTRO_UBUNTU;
            dusmanlar[0].x = 750.0f;
            dusmanlar[0].y = 430.0f;
            dusmanlar[0].hizY = 0.0f;
            dusmanlar[0].genislik = 70.0f;
            dusmanlar[0].yukseklik = 70.0f;
            dusmanlar[0].can = 350;
            dusmanlar[0].maxCan = 350;
            dusmanlar[0].hiz = 2.8f;
            dusmanlar[0].yon = -1;
            dusmanlar[0].sinirSol = 200.0f;
            dusmanlar[0].sinirSag = 1300.0f;
            dusmanlar[0].atesSayaci = 1.2f;
            dusmanlar[0].isBoss = true;
            dusmanlar[0].hayattaMi = true;
            dusmanlar[0].bossDurum = 0;
            dusmanlar[0].yetenekSayaci = 2.5f;
            dusmanlar[0].durumSuresi = 0.0f;
            dusmanlar[0].bossParryAktif = false;
            return;
        }

        dalgaDusmanSayisi = (mod == MOD_MINI_FIGHT) ? 4 : (2 + dalgaNo * 2);
        if (dalgaDusmanSayisi > MAX_DUSMAN) dalgaDusmanSayisi = MAX_DUSMAN;

        hayattakiDusman = dalgaDusmanSayisi;

        for (int i = 0; i < dalgaDusmanSayisi; i++) {
            dusmanlar[i].isBoss = false;
            dusmanlar[i].hayattaMi = true;
            dusmanlar[i].yon = (i % 2 == 0) ? 1 : -1;
            dusmanlar[i].hizY = 0.0f;
            dusmanlar[i].bossDurum = 0;
            dusmanlar[i].ozelYetenekSayaci = (float)GetRandomValue(10, 30) / 10.0f;

            int distroSecim = GetRandomValue(0, 5); 
            dusmanlar[i].distro = (DistroTipi)distroSecim;

            switch (dusmanlar[i].distro) {
                case DISTRO_DEBIAN:
                    dusmanlar[i].genislik = 50.0f;
                    dusmanlar[i].yukseklik = 50.0f;
                    dusmanlar[i].maxCan = 60 + (dalgaNo * 25);
                    dusmanlar[i].hiz = 1.2f;
                    dusmanlar[i].atesSayaci = 3.5f;
                    break;
                case DISTRO_KALI:
                    dusmanlar[i].genislik = 38.0f;
                    dusmanlar[i].yukseklik = 38.0f;
                    dusmanlar[i].maxCan = 25 + (dalgaNo * 10);
                    dusmanlar[i].hiz = 3.6f;
                    dusmanlar[i].atesSayaci = 2.8f;
                    break;
                case DISTRO_FEDORA:
                    dusmanlar[i].genislik = 40.0f;
                    dusmanlar[i].yukseklik = 40.0f;
                    dusmanlar[i].maxCan = 30 + (dalgaNo * 12);
                    dusmanlar[i].hiz = 1.9f;
                    dusmanlar[i].atesSayaci = 1.4f;
                    break;
                case DISTRO_POPOS:
                    dusmanlar[i].genislik = 40.0f;
                    dusmanlar[i].yukseklik = 40.0f;
                    dusmanlar[i].maxCan = 35 + (dalgaNo * 14);
                    dusmanlar[i].hiz = 2.2f;
                    dusmanlar[i].atesSayaci = 2.2f;
                    break;
                case DISTRO_ZORIN:
                    dusmanlar[i].genislik = 40.0f;
                    dusmanlar[i].yukseklik = 40.0f;
                    dusmanlar[i].maxCan = 30 + (dalgaNo * 12);
                    dusmanlar[i].hiz = 2.9f;
                    dusmanlar[i].atesSayaci = 2.0f;
                    break;
                default:
                    dusmanlar[i].genislik = 40.0f;
                    dusmanlar[i].yukseklik = 40.0f;
                    dusmanlar[i].maxCan = 35 + (dalgaNo * 15);
                    dusmanlar[i].hiz = 2.0f;
                    dusmanlar[i].atesSayaci = 2.2f;
                    break;
            }
            dusmanlar[i].can = dusmanlar[i].maxCan;

            if (i % 3 == 0) {
                dusmanlar[i].x = 320.0f + (i * 20);
                dusmanlar[i].y = 380.0f - dusmanlar[i].yukseklik;
                dusmanlar[i].sinirSol = 300.0f;
                dusmanlar[i].sinirSag = 450.0f;
            } else if (i % 3 == 1) {
                dusmanlar[i].x = 580.0f + (i * 20);
                dusmanlar[i].y = 280.0f - dusmanlar[i].yukseklik;
                dusmanlar[i].sinirSol = 550.0f;
                dusmanlar[i].sinirSag = 750.0f;
            } else {
                dusmanlar[i].x = 900.0f + (i * 30);
                dusmanlar[i].y = 500.0f - dusmanlar[i].yukseklik;
                dusmanlar[i].sinirSol = 200.0f;
                dusmanlar[i].sinirSag = 1400.0f;
            }
        }
    };

    auto KartlariOlustur = [&](int dalgaNo) {
        kartSecim = 0;
        Kart erkenHavuz[] = {
            { "HIZLI AYAKLAR", "Kosma hizi kalici olarak +0.8 artar.", 1 },
            { "GENIS SARJOR", "Sarjor kapasitesi +3 mermi genisler.", 2 },
            { "SERI DOLDURMA", "Sarjor doldurma suresi 0.25 sn kisalir.", 3 },
            { "CELIK YELEK", "Maksimum Zirh +25 artar ve zırh tazelenir.", 4 },
            { "CANLILIK", "Maksimum Can +20 artar ve can toparlanir.", 5 },
            { "KONDISYON", "Ziplama staminasi %20 daha hizli dolar.", 6 }
        };

        Kart gecHavuz[] = {
            { "AGIR MUHIMMAT", "Mermi hasari +15 artar.", 7 },
            { "BETON YUMRUK", "Yakin dovus hasari +25 artar.", 8 },
            { "TEKTONIK SLAM", "Yere cakilma (Slam) hasari +40 artar.", 9 },
            { "EKSTRA DASH", "Dash hakki +1 artar ve dolumu hizlanir.", 10 },
            { "CELIK GOVDE", "Max Can +40 ve Max Zirh +40 buyur.", 11 },
            { "DELICI SARJOR", "Sarjor kapasitesi +6 ve Mermi hasari +8 artar.", 12 }
        };

        bool ileriDalga = (dalgaNo >= 4);
        int secilenIndexler[3] = { -1, -1, -1 };

        for (int i = 0; i < 3; i++) {
            int rnd;
            bool benzersiz;
            do {
                benzersiz = true;
                rnd = GetRandomValue(0, 5);
                for (int j = 0; j < i; j++) {
                    if (secilenIndexler[j] == rnd) benzersiz = false;
                }
            } while (!benzersiz);

            secilenIndexler[i] = rnd;
            sunulanKartlar[i] = ileriDalga ? gecHavuz[rnd] : erkenHavuz[rnd];
        }
    };

    auto KartiUygula = [&](int kartId) {
        switch (kartId) {
            case 1: hiz += 0.8f; break;
            case 2: sarjorKapasite += 3; sarjorMermi += 3; break;
            case 3: reloadSuresi = (reloadSuresi > 0.4f) ? (reloadSuresi - 0.25f) : 0.4f; break;
            case 4: maxZirh += 25; mevcutZirh = maxZirh; break;
            case 5: maxCan += 20; mevcutCan = (mevcutCan + 30 > maxCan) ? maxCan : mevcutCan + 30; break;
            case 6: staminaDolmaSuresi = (staminaDolmaSuresi > 0.8f) ? (staminaDolmaSuresi - 0.3f) : 0.8f; break;
            case 7: oyuncuMermiHasari += 15; break;
            case 8: oyuncuYumrukHasari += 25; break;
            case 9: oyuncuSlamHasari += 40; break;
            case 10: maxDashHakki += 1; kalanDash = maxDashHakki; dashDolmaSuresi *= 0.85f; break;
            case 11: maxCan += 40; maxZirh += 40; mevcutCan = maxCan; mevcutZirh = maxZirh; break;
            case 12: sarjorKapasite += 6; oyuncuMermiHasari += 8; break;
        }
    };

    auto OyunuBaslat = [&](OyunModu mod) {
        seciliMod = mod;
        karakterX = 100.0f;
        karakterY = 400.0f;
        hizY = 0.0f;
        hiz = 5.0f;
        maxCan = 100;
        mevcutCan = 100;
        maxZirh = 100;
        mevcutZirh = 50;
        sarjorKapasite = 12;
        sarjorMermi = 12;
        yedekMermi = 60;
        reloadSuresi = 1.2f;
        oyuncuMermiHasari = 25;
        oyuncuYumrukHasari = 35;
        oyuncuSlamHasari = 60;
        maxZiplamaHakki = 5;
        kalanHak = 5;
        maxDashHakki = 3;
        kalanDash = 3;
        staminaDolmaSuresi = 1.7f;
        dashDolmaSuresi = 1.5f;

        reloadYapiyor = false;
        dashAtiyorMu = false;
        slamYapiyorMusu = false;
        parryAktif = false;
        parrySayaci = 0.0f;
        parryBeklemeSuresi = 0.0f;
        beyazFlashSuresi = 0.0f;
        dalga = 1;
        yeniDalgaBekleniyor = false;
        bossFaz = 1;
        faz2BildirimSayaci = 0.0f;
        kosuSuresi = 0.0f;
        zaferObjesiAktif = false;

        for (int i = 0; i < MAX_OYUNCU_MERMI; i++) oyuncuMermiler[i].aktif = false;
        for (int i = 0; i < MAX_DUSMAN_MERMI; i++) dusmanMermiler[i].aktif = false;
        for (int i = 0; i < MAX_ITEMS; i++) itemler[i].aktif = false;
        for (int i = 0; i < MAX_DIKEN; i++) dikenler[i].aktif = false;
        for (int i = 0; i < MAX_PARCACIK; i++) parcaciklar[i].aktif = false;
        for (int i = 0; i < MAX_YAZI; i++) ucanYazilar[i].aktif = false;
        for (int i = 0; i < MAX_SOK; i++) sokDalgalari[i].aktif = false;

        DalgaUret(dalga, seciliMod);
        durum = OYUN_AKTIF;
    };

    auto DusmanaHasarVer = [&](int index, int hasar) {
        dusmanlar[index].can -= hasar;
        GuvenliSesCal(sndHit);

        // Uçan Hasar Sayısı & Kıvılcım
        YaziEkle(dusmanlar[index].x + 10, dusmanlar[index].y - 10, TextFormat("-%d", hasar), ORANGE);
        ParcacikPatlat(dusmanlar[index].x + 20, dusmanlar[index].y + 20, YELLOW, 5, 3.0f);

        if (dusmanlar[index].can <= 0) {
            if (dusmanlar[index].isBoss && bossFaz == 1) {
                bossFaz = 2;
                dusmanlar[index].can = 550;
                dusmanlar[index].maxCan = 550;
                dusmanlar[index].genislik = 95.0f;
                dusmanlar[index].yukseklik = 95.0f;
                dusmanlar[index].y = 500.0f - dusmanlar[index].yukseklik;
                dusmanlar[index].hiz = 3.8f;
                dusmanlar[index].bossDurum = 0;
                dusmanlar[index].yetenekSayaci = 1.2f;
                dusmanlar[index].bossParryAktif = false;
                
                mevcutCan = (mevcutCan + 30 > maxCan) ? maxCan : mevcutCan + 30;
                mevcutZirh = (mevcutZirh + 30 > maxZirh) ? maxZirh : mevcutZirh + 30;
                sarjorMermi = sarjorKapasite;
                reloadYapiyor = false;

                sarsintiSuresi = 0.8f;
                beyazFlashSuresi = 0.25f;
                faz2BildirimSayaci = 2.5f;

                SokDalgasiYarat(dusmanlar[index].x + 47, dusmanlar[index].y + 47, 180.0f, RED);
                ParcacikPatlat(dusmanlar[index].x + 47, dusmanlar[index].y + 47, MAROON, 40, 7.0f);
            } else {
                dusmanlar[index].hayattaMi = false;
                hayattakiDusman--;

                // Dağıtım Renginde Ölüm Patlaması
                Color pRenk = GREEN;
                switch (dusmanlar[index].distro) {
                    case DISTRO_DEBIAN: pRenk = RED; break;
                    case DISTRO_FEDORA: pRenk = BLUE; break;
                    case DISTRO_KALI:   pRenk = DARKBLUE; break;
                    case DISTRO_POPOS:  pRenk = SKYBLUE; break;
                    case DISTRO_ZORIN:  pRenk = LIGHTGRAY; break;
                    case DISTRO_UBUNTU: pRenk = ORANGE; break;
                    default: break;
                }
                ParcacikPatlat(dusmanlar[index].x + 20, dusmanlar[index].y + 20, pRenk, 25, 5.5f);

                if (dusmanlar[index].isBoss) {
                    zaferObjesiAktif = true;
                    zaferObjesi.x = dusmanlar[index].x + 30.0f;
                    zaferObjesi.y = 465.0f; 

                    sarsintiSuresi = 0.6f;
                    beyazFlashSuresi = 0.2f;
                    SokDalgasiYarat(zaferObjesi.x + 15, zaferObjesi.y + 15, 120.0f, GOLD);

                    for (int k = 0; k < 5; k++) {
                        for (int m = 0; m < MAX_ITEMS; m++) {
                            if (!itemler[m].aktif) {
                                itemler[m].x = dusmanlar[index].x + (k * 20);
                                itemler[m].y = dusmanlar[index].y + 20;
                                itemler[m].tip = (ItemTipi)(k % 3);
                                itemler[m].miktar = (itemler[m].tip == ITEM_MERMI) ? 12 : 0;
                                itemler[m].aktif = true;
                                break;
                            }
                        }
                    }
                } else if (GetRandomValue(1, 100) <= 60) {
                    for (int k = 0; k < MAX_ITEMS; k++) {
                        if (!itemler[k].aktif) {
                            itemler[k].x = dusmanlar[index].x + 10;
                            itemler[k].y = dusmanlar[index].y + 10;
                            itemler[k].tip = (ItemTipi)GetRandomValue(0, 2);
                            itemler[k].miktar = (itemler[k].tip == ITEM_MERMI) ? GetRandomValue(1, 12) : 0;
                            itemler[k].aktif = true;
                            break;
                        }
                    }
                }
            }
        }
    };

    while (true) {
        float dt = GetFrameTime();

        // 1. ANA MENÜ
        if (durum == MENU_ANA) {
            if (IsKeyPressed(KEY_UP)) anaMenuSecim = (anaMenuSecim - 1 + 2) % 2;
            if (IsKeyPressed(KEY_DOWN)) anaMenuSecim = (anaMenuSecim + 1) % 2;

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (anaMenuSecim == 0) durum = MENU_MOD_SEC;
                else if (anaMenuSecim == 1) break;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("DISTRO WARS: LAST ARCH PROTECTOR", 40, 130, 36, SKYBLUE);
            DrawText("HELP ARCH FOR THIS WAR", 275, 185, 18, LIGHTGRAY);

            DrawText(anaMenuSecim == 0 ? "> OYNA <" : "  OYNA  ", 340, 280, 26, (anaMenuSecim == 0 ? GOLD : WHITE));
            DrawText(anaMenuSecim == 1 ? "> CIKIS <" : "  CIKIS  ", 335, 340, 26, (anaMenuSecim == 1 ? GOLD : WHITE));

            DrawRectangleLines(200, 420, 400, 75, DARKGRAY);
            DrawText(TextFormat("Endless En Yuksek Dalga: %d", enYuksekDalga), 220, 432, 16, YELLOW);
            if (enIyiSure > 0.0f) {
                int dk = (int)enIyiSure / 60;
                int sn = (int)enIyiSure % 60;
                DrawText(TextFormat("Ubuntu Boss En Iyi Sure: %02d:%02d | Tekrar: %d", dk, sn, miniFightTekrar), 220, 460, 14, GREEN);
            } else {
                DrawText(TextFormat("Ubuntu Boss Henuz Kesilmedi | Tekrar: %d", miniFightTekrar), 220, 460, 14, RED);
            }

            DrawText("[YUKARI / ASAGI] Sec | [ENTER] Onayla", 240, 530, 16, GRAY);
            EndDrawing();
            continue;
        }

        // 2. MOD MENÜSÜ
        if (durum == MENU_MOD_SEC) {
            if (IsKeyPressed(KEY_UP)) modMenuSecim = (modMenuSecim - 1 + 2) % 2;
            if (IsKeyPressed(KEY_DOWN)) modMenuSecim = (modMenuSecim + 1) % 2;
            if (IsKeyPressed(KEY_ESCAPE)) durum = MENU_ANA;

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (modMenuSecim == 0) OyunuBaslat(MOD_ENDLESS);
                else if (modMenuSecim == 1) OyunuBaslat(MOD_MINI_FIGHT);
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("MOD SECIMI", 310, 130, 36, SKYBLUE);

            DrawText(modMenuSecim == 0 ? "> ENDLESS WAVE <" : "  ENDLESS WAVE  ", 260, 250, 24, (modMenuSecim == 0 ? GOLD : WHITE));
            DrawText(TextFormat("Tum distrolar akın akın gelir | Rekor: %d. Dalga", enYuksekDalga), 205, 285, 14, YELLOW);

            DrawText(modMenuSecim == 1 ? "> MINI FIGHT (UBUNTU BOSS) <" : "  MINI FIGHT (UBUNTU BOSS)  ", 190, 350, 24, (modMenuSecim == 1 ? GOLD : WHITE));
            DrawText("Distro ordusunu temizle ve 2 Fazli Ubuntu Boss'u devir!", 200, 385, 14, GRAY);

            DrawText("[ESC] Geri | [ENTER] Baslat", 290, 520, 16, DARKGRAY);
            EndDrawing();
            continue;
        }

        // 3. ROGUELIKE YÜKSELTME
        if (durum == OYUN_YUKSELTME) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))  kartSecim = (kartSecim - 1 + 3) % 3;
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) kartSecim = (kartSecim + 1) % 3;

            if (IsKeyPressed(KEY_ONE))   { KartiUygula(sunulanKartlar[0].id); dalga++; DalgaUret(dalga, seciliMod); durum = OYUN_AKTIF; continue; }
            if (IsKeyPressed(KEY_TWO))   { KartiUygula(sunulanKartlar[1].id); dalga++; DalgaUret(dalga, seciliMod); durum = OYUN_AKTIF; continue; }
            if (IsKeyPressed(KEY_THREE)) { KartiUygula(sunulanKartlar[2].id); dalga++; DalgaUret(dalga, seciliMod); durum = OYUN_AKTIF; continue; }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                KartiUygula(sunulanKartlar[kartSecim].id);
                dalga++;
                DalgaUret(dalga, seciliMod);
                durum = OYUN_AKTIF;
                continue;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawRectangle(0, 0, 800, 600, Fade(BLACK, 0.7f));
            DrawText(TextFormat("DALGA %d TAMAMLANDI!", dalga), 250, 80, 32, YELLOW);
            DrawText("Arch Linux Sistemin Icin Bir Modul Sec:", 240, 130, 18, LIGHTGRAY);

            for (int i = 0; i < 3; i++) {
                int kartX = 70 + (i * 230);
                int kartY = 180;
                int kartW = 200;
                int kartH = 260;

                bool secili = (kartSecim == i);
                Color cerceveRengi = secili ? GOLD : DARKGRAY;
                Color arkaplanRengi = secili ? Fade(GOLD, 0.15f) : Fade(DARKGRAY, 0.2f);

                DrawRectangle(kartX, kartY, kartW, kartH, arkaplanRengi);
                DrawRectangleLines(kartX, kartY, kartW, kartH, cerceveRengi);

                DrawText(TextFormat("[%d]", i + 1), kartX + 90, kartY + 15, 16, GRAY);
                DrawText(sunulanKartlar[i].baslik, kartX + 15, kartY + 45, 15, secili ? GOLD : WHITE);
                DrawLine(kartX + 15, kartY + 75, kartX + kartW - 15, kartY + 75, cerceveRengi);
                DrawText(sunulanKartlar[i].aciklama, kartX + 15, kartY + 95, 13, LIGHTGRAY);

                if (secili) DrawText("> SEC <", kartX + 70, kartY + 225, 14, GOLD);
            }

            DrawText("[SOL / SAG] veya [1, 2, 3] ile Sec | [ENTER] Onayla", 220, 480, 15, GRAY);
            EndDrawing();
            continue;
        }

        // 4. ÖLÜM EKRANI
        if (durum == OYUN_BITTI) {
            if (IsKeyPressed(KEY_R)) OyunuBaslat(seciliMod);
            if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) durum = MENU_ANA;

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("KERNEL PANIC! OYUN BITTI", 150, 180, 38, RED);

            if (seciliMod == MOD_ENDLESS) {
                DrawText(TextFormat("Dayanilan Dalga: %d", dalga), 320, 250, 20, YELLOW);
                DrawText(TextFormat("En Yuksek Rekor: %d", enYuksekDalga), 320, 280, 18, GOLD);
            } else {
                int dk = (int)kosuSuresi / 60;
                int sn = (int)kosuSuresi % 60;
                DrawText(TextFormat("Gecen Sure: %02d:%02d", dk, sn), 335, 250, 20, LIGHTGRAY);
                DrawText(TextFormat("Toplam Tekrar (Olum): %d", miniFightTekrar), 285, 285, 20, RED);
            }

            DrawText("[R] Yeniden Basla | [M] Ana Menu", 250, 400, 18, LIGHTGRAY);
            EndDrawing();
            continue;
        }

        // 5. ZAFER EKRANI
        if (durum == OYUN_KAZANDIN) {
            if (IsKeyPressed(KEY_R)) OyunuBaslat(seciliMod);
            if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) durum = MENU_ANA;

            int dk = (int)kosuSuresi / 60;
            int sn = (int)kosuSuresi % 60;
            int salise = (int)((kosuSuresi - (int)kosuSuresi) * 100);

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("ARCH USTUNLUK SAGLADI! ZAFER!", 140, 160, 34, GREEN);
            DrawText("Ubuntu Boss tamamen etkisiz hale getirildi!", 220, 220, 18, LIGHTGRAY);

            DrawText(TextFormat("Bitirme Suren: %02d:%02d.%02d", dk, sn, salise), 280, 270, 22, GOLD);
            if (enIyiSure > 0.0f) {
                int rekDk = (int)enIyiSure / 60;
                int rekSn = (int)enIyiSure % 60;
                DrawText(TextFormat("En Iyi Sure: %02d:%02d", rekDk, rekSn), 330, 305, 18, YELLOW);
            }

            DrawText("Tekrar sayaci sifirlandi (0)!", 300, 345, 18, SKYBLUE);
            DrawText("[R] Yeniden Oyna | [M] Ana Menu", 255, 420, 18, GOLD);
            EndDrawing();
            continue;
        }

        // 6. AKTİF OYUN GÜNCELLEMELERİ
        if (IsKeyPressed(KEY_ESCAPE)) { durum = MENU_ANA; continue; }

        if (mevcutCan <= 0) {
            if (seciliMod == MOD_MINI_FIGHT) miniFightTekrar++;
            else if (seciliMod == MOD_ENDLESS && dalga > enYuksekDalga) enYuksekDalga = dalga;
            IstatistikleriKaydet(miniFightTekrar, enIyiSure, enYuksekDalga);
            durum = OYUN_BITTI;
            continue;
        }

        kosuSuresi += dt;
        if (beyazFlashSuresi > 0) beyazFlashSuresi -= dt;
        if (faz2BildirimSayaci > 0) faz2BildirimSayaci -= dt;

        if (seciliMod == MOD_ENDLESS && dalga > enYuksekDalga) {
            enYuksekDalga = dalga;
            IstatistikleriKaydet(miniFightTekrar, enIyiSure, enYuksekDalga);
        }

        if (hayattakiDusman <= 0 && !yeniDalgaBekleniyor && !zaferObjesiAktif) {
            if (seciliMod == MOD_ENDLESS) {
                KartlariOlustur(dalga);
                durum = OYUN_YUKSELTME;
                continue;
            } 
            else if (seciliMod == MOD_MINI_FIGHT && dalga == 1) {
                yeniDalgaBekleniyor = true;
                dalgaArasiSayac = 3.0f;
            }
        }

        if (yeniDalgaBekleniyor) {
            dalgaArasiSayac -= dt;
            if (dalgaArasiSayac <= 0.0f) {
                dalga++;
                DalgaUret(dalga, seciliMod);
                yeniDalgaBekleniyor = false;
            }
        }

        // Parry
        if (parryBeklemeSuresi > 0) parryBeklemeSuresi -= dt;
        if (IsKeyPressed(KEY_F) && parryBeklemeSuresi <= 0.0f) {
            parryAktif = true;
            parrySayaci = 0.22f;          
            parryBeklemeSuresi = 0.55f;   
        }
        if (parryAktif) {
            parrySayaci -= dt;
            if (parrySayaci <= 0.0f) parryAktif = false;
        }

        // Şarjör
        if (IsKeyPressed(KEY_R) && !reloadYapiyor && sarjorMermi < sarjorKapasite && yedekMermi > 0) {
            reloadYapiyor = true;
            reloadTimer = reloadSuresi;
        }
        if (reloadYapiyor) {
            reloadTimer -= dt;
            if (reloadTimer <= 0.0f) {
                int eksik = sarjorKapasite - sarjorMermi;
                int basilacak = (yedekMermi >= eksik) ? eksik : yedekMermi;
                sarjorMermi += basilacak;
                yedekMermi -= basilacak;
                reloadYapiyor = false;
            }
        }

        // Dash
        if ((IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) && kalanDash > 0 && !dashAtiyorMu) {
            dashAtiyorMu = true;
            dashKalanSure = dashSuresi;
            dashYonu = karakterYon;
            hizY = 0.0f;
            kalanDash--;
            GuvenliSesCal(sndDash);
            ParcacikPatlat(karakterX + 20, karakterY + 20, SKYBLUE, 10, 3.0f);
        }

        if (dashAtiyorMu) {
            karakterX += dashHizi * dashYonu;
            dashKalanSure -= dt;
            // Dash esnasında arkada toz/enerji partikülleri bırak
            ParcacikPatlat(karakterX + 20, karakterY + 25, SKYBLUE, 2, 1.5f);
            if (dashKalanSure <= 0.0f) dashAtiyorMu = false;
        } 
        else {
            bool egiliyorMu = false;
            if (IsKeyDown(KEY_DOWN) && yerdeMi) egiliyorMu = true;

            float anlikHiz = egiliyorMu ? (hiz / 2.0f) : hiz; 
            if (IsKeyDown(KEY_RIGHT)) { karakterX += anlikHiz; karakterYon = 1; }
            if (IsKeyDown(KEY_LEFT))  { karakterX -= anlikHiz; karakterYon = -1; }

            if (IsKeyPressed(KEY_UP) && kalanHak > 0 && !egiliyorMu) {
                hizY = ziplamaGucu; 
                kalanHak--;     
                yerdeMi = false;
                ParcacikPatlat(karakterX + 20, karakterY + 40, LIGHTGRAY, 4, 2.0f);
            }

            if (!yerdeMi && IsKeyPressed(KEY_DOWN)) {
                hizY = 50.0f;
                slamYapiyorMusu = true;
            }

            karakterY += hizY; 
            hizY += yercekimi; 
        }

        if (karakterX < 0) karakterX = 0;
        if (karakterX > 1600 - karakterGenislik) karakterX = 1600 - karakterGenislik;

        // Stamina Dolumları
        if (kalanDash < maxDashHakki) {
            dashTimer += dt;
            if (dashTimer >= dashDolmaSuresi) { kalanDash++; dashTimer = 0.0f; }
        }
        if (kalanHak < maxZiplamaHakki) {
            staminaTimer += dt;
            if (staminaTimer >= staminaDolmaSuresi) { kalanHak++; staminaTimer = 0.0f; }
        }

        // Oyuncu Ateş Etme
        if (IsKeyPressed(KEY_SPACE) && !reloadYapiyor) {
            if (sarjorMermi > 0) {
                for (int i = 0; i < MAX_OYUNCU_MERMI; i++) {
                    if (!oyuncuMermiler[i].aktif) {
                        oyuncuMermiler[i].x = karakterX + (karakterYon > 0 ? karakterGenislik : -10);
                        oyuncuMermiler[i].y = karakterY + (karakterYukseklik / 2.0f);
                        oyuncuMermiler[i].hizX = 15.0f * karakterYon; 
                        oyuncuMermiler[i].yansitildi = false;
                        oyuncuMermiler[i].aktif = true;
                        sarjorMermi--;

                        GuvenliSesCal(sndAtes);
                        // Namlu ateşi partikülü
                        ParcacikPatlat(oyuncuMermiler[i].x, oyuncuMermiler[i].y, GOLD, 5, 2.5f);
                        break;
                    }
                }
            } else if (yedekMermi > 0) {
                reloadYapiyor = true;
                reloadTimer = reloadSuresi;
            }
        }

        // Yumruk
        if (IsKeyPressed(KEY_ENTER)) {
            yumrukEfektSuresi = 0.15f; 
            Rectangle yumrukKutusu = { 
                karakterYon > 0 ? karakterX + karakterGenislik : karakterX - 35, 
                karakterY + 5, 
                35, 
                (float)karakterYukseklik - 10 
            };

            for (int i = 0; i < dalgaDusmanSayisi; i++) {
                if (dusmanlar[i].hayattaMi) {
                    Rectangle dusmanKutusu = { dusmanlar[i].x, dusmanlar[i].y, dusmanlar[i].genislik, dusmanlar[i].yukseklik };
                    if (CheckCollisionRecs(yumrukKutusu, dusmanKutusu)) {
                        if (dusmanlar[i].isBoss && dusmanlar[i].bossParryAktif) {
                            mevcutCan -= 20; 
                            sarsintiSuresi = 0.3f;
                            beyazFlashSuresi = 0.05f;
                            GuvenliSesCal(sndParry);
                            continue;
                        }
                        DusmanaHasarVer(i, oyuncuYumrukHasari);
                    }
                }
            }
        }
        if (yumrukEfektSuresi > 0) yumrukEfektSuresi -= dt;

        // Oyuncu Mermileri
        for (int i = 0; i < MAX_OYUNCU_MERMI; i++) {
            if (oyuncuMermiler[i].aktif) {
                oyuncuMermiler[i].x += oyuncuMermiler[i].hizX;
                if (oyuncuMermiler[i].x < 0 || oyuncuMermiler[i].x > 1600) oyuncuMermiler[i].aktif = false;

                Rectangle mermiKutusu = { oyuncuMermiler[i].x, oyuncuMermiler[i].y, 8, 4 };
                for (int j = 0; j < dalgaDusmanSayisi; j++) {
                    if (dusmanlar[j].hayattaMi) {
                        Rectangle dusmanKutusu = { dusmanlar[j].x, dusmanlar[j].y, dusmanlar[j].genislik, dusmanlar[j].yukseklik };
                        if (CheckCollisionRecs(mermiKutusu, dusmanKutusu)) {
                            if (dusmanlar[j].isBoss && dusmanlar[j].bossParryAktif) {
                                for (int m = 0; m < MAX_DUSMAN_MERMI; m++) {
                                    if (!dusmanMermiler[m].aktif) {
                                        dusmanMermiler[m].x = oyuncuMermiler[i].x;
                                        dusmanMermiler[m].y = oyuncuMermiler[i].y;
                                        dusmanMermiler[m].hizX = -oyuncuMermiler[i].hizX * 0.9f;
                                        dusmanMermiler[m].aktif = true;
                                        break;
                                    }
                                }
                                oyuncuMermiler[i].aktif = false;
                                sarsintiSuresi = 0.15f;
                                GuvenliSesCal(sndParry);
                                break;
                            }

                            oyuncuMermiler[i].aktif = false; 
                            DusmanaHasarVer(j, oyuncuMermiHasari);
                            break;
                        }
                    }
                }
            }
        }

        // Düşman Yapay Zekası
        for (int i = 0; i < dalgaDusmanSayisi; i++) {
            if (!dusmanlar[i].hayattaMi) continue; 

            if (dusmanlar[i].isBoss) {
                dusmanlar[i].yetenekSayaci -= dt;

                if (dusmanlar[i].yetenekSayaci <= 0.0f && dusmanlar[i].bossDurum == 0) {
                    int secim = GetRandomValue(1, 3);
                    dusmanlar[i].bossDurum = secim;

                    if (secim == 1) { 
                        dusmanlar[i].durumSuresi = (bossFaz == 2) ? 0.45f : 0.35f;
                        dusmanlar[i].yon = (karakterX > dusmanlar[i].x) ? 1 : -1;
                    } 
                    else if (secim == 2) { 
                        dusmanlar[i].hizY = (bossFaz == 2) ? -18.0f : -15.0f; 
                        dusmanlar[i].durumSuresi = 1.2f;
                    }
                    else if (secim == 3) { 
                        dusmanlar[i].durumSuresi = (bossFaz == 2) ? 1.3f : 1.0f;
                        dusmanlar[i].bossParryAktif = true;
                    }
                    dusmanlar[i].yetenekSayaci = (bossFaz == 2) ? 
                        ((float)GetRandomValue(15, 28) / 10.0f) : 
                        ((float)GetRandomValue(25, 45) / 10.0f);
                }

                if (dusmanlar[i].bossDurum == 1) {
                    float dashHiz = (bossFaz == 2) ? 20.0f : 14.0f;
                    dusmanlar[i].x += dashHiz * dusmanlar[i].yon;
                    dusmanlar[i].durumSuresi -= dt;
                    if (dusmanlar[i].durumSuresi <= 0.0f) dusmanlar[i].bossDurum = 0;
                }
                else if (dusmanlar[i].bossDurum == 2) {
                    dusmanlar[i].y += dusmanlar[i].hizY;
                    dusmanlar[i].hizY += yercekimi;

                    if (dusmanlar[i].hizY > 2.0f) {
                        dusmanlar[i].hizY = (bossFaz == 2) ? 28.0f : 22.0f; 
                    }

                    if (dusmanlar[i].y >= 500 - dusmanlar[i].yukseklik) {
                        dusmanlar[i].y = 500 - dusmanlar[i].yukseklik;
                        dusmanlar[i].hizY = 0.0f;
                        dusmanlar[i].bossDurum = 0;
                        sarsintiSuresi = (bossFaz == 2) ? 0.7f : 0.4f;

                        GuvenliSesCal(sndSlam);
                        SokDalgasiYarat(dusmanlar[i].x + (dusmanlar[i].genislik / 2.0f), 500, 140.0f, RED);

                        int dikenSayisi = (bossFaz == 2) ? 12 : 6;
                        DikenCikar(dusmanlar[i].x + (dusmanlar[i].genislik / 2.0f), 500 - 32, dikenSayisi);
                    }
                }
                else if (dusmanlar[i].bossDurum == 3) {
                    dusmanlar[i].durumSuresi -= dt;
                    if (dusmanlar[i].durumSuresi <= 0.0f) {
                        dusmanlar[i].bossParryAktif = false;
                        dusmanlar[i].bossDurum = 0;
                    }
                }
                else {
                    dusmanlar[i].x += dusmanlar[i].hiz * dusmanlar[i].yon;
                    if (dusmanlar[i].x < dusmanlar[i].sinirSol || dusmanlar[i].x > dusmanlar[i].sinirSag) {
                        dusmanlar[i].yon *= -1; 
                    }
                }

                dusmanlar[i].atesSayaci -= dt;
                if (dusmanlar[i].atesSayaci <= 0.0f && dusmanlar[i].bossDurum != 3) {
                    dusmanlar[i].atesSayaci = (bossFaz == 2) ? 0.9f : 1.3f;
                    int atisYonu = (karakterX > dusmanlar[i].x) ? 1 : -1;

                    if (bossFaz == 2) {
                        float dikeyOfsetler[] = { -22.0f, 0.0f, 22.0f };
                        for (int k = 0; k < 3; k++) {
                            for (int m = 0; m < MAX_DUSMAN_MERMI; m++) {
                                if (!dusmanMermiler[m].aktif) {
                                    dusmanMermiler[m].x = dusmanlar[i].x + (atisYonu > 0 ? dusmanlar[i].genislik : -8);
                                    dusmanMermiler[m].y = dusmanlar[i].y + (dusmanlar[i].yukseklik / 2.0f) + dikeyOfsetler[k];
                                    dusmanMermiler[m].hizX = 9.0f * atisYonu;
                                    dusmanMermiler[m].aktif = true;
                                    break;
                                }
                            }
                        }
                    } else {
                        for (int m = 0; m < MAX_DUSMAN_MERMI; m++) {
                            if (!dusmanMermiler[m].aktif) {
                                dusmanMermiler[m].x = dusmanlar[i].x + (atisYonu > 0 ? dusmanlar[i].genislik : -6);
                                dusmanMermiler[m].y = dusmanlar[i].y + (dusmanlar[i].yukseklik / 2.0f);
                                dusmanMermiler[m].hizX = 8.0f * atisYonu;
                                dusmanMermiler[m].aktif = true;
                                break;
                            }
                        }
                    }
                }
            } 
            else {
                if (dusmanlar[i].distro == DISTRO_POPOS) {
                    dusmanlar[i].ozelYetenekSayaci -= dt;
                    if (dusmanlar[i].ozelYetenekSayaci <= 0.0f && dusmanlar[i].hizY == 0.0f) {
                        dusmanlar[i].hizY = -10.0f;
                        dusmanlar[i].ozelYetenekSayaci = (float)GetRandomValue(20, 35) / 10.0f;
                    }
                    dusmanlar[i].y += dusmanlar[i].hizY;
                    dusmanlar[i].hizY += yercekimi;

                    if (dusmanlar[i].y >= 500 - dusmanlar[i].yukseklik) {
                        dusmanlar[i].y = 500 - dusmanlar[i].yukseklik;
                        dusmanlar[i].hizY = 0.0f;
                    }
                }

                float gercekHiz = dusmanlar[i].hiz;
                if (dusmanlar[i].distro == DISTRO_KALI) {
                    float mesafe = fabsf(karakterX - dusmanlar[i].x);
                    if (mesafe < 220.0f) {
                        gercekHiz = 5.2f;
                        dusmanlar[i].yon = (karakterX > dusmanlar[i].x) ? 1 : -1;
                    }
                }

                dusmanlar[i].x += gercekHiz * dusmanlar[i].yon;
                if (dusmanlar[i].x < dusmanlar[i].sinirSol || dusmanlar[i].x > dusmanlar[i].sinirSag) {
                    dusmanlar[i].yon *= -1; 
                }

                dusmanlar[i].atesSayaci -= dt;
                if (dusmanlar[i].atesSayaci <= 0.0f) {
                    dusmanlar[i].atesSayaci = (dusmanlar[i].distro == DISTRO_FEDORA) ? 1.4f : ((float)GetRandomValue(20, 35) / 10.0f);
                    int atisYonu = (karakterX > dusmanlar[i].x) ? 1 : -1;

                    for (int m = 0; m < MAX_DUSMAN_MERMI; m++) {
                        if (!dusmanMermiler[m].aktif) {
                            dusmanMermiler[m].x = dusmanlar[i].x + (atisYonu > 0 ? dusmanlar[i].genislik : -6);
                            dusmanMermiler[m].y = dusmanlar[i].y + (dusmanlar[i].yukseklik / 2.0f);
                            float mHiz = (dusmanlar[i].distro == DISTRO_FEDORA) ? 8.5f : 6.5f;
                            dusmanMermiler[m].hizX = mHiz * atisYonu;
                            dusmanMermiler[m].aktif = true;
                            break;
                        }
                    }
                }
            }
        }

        // Diken Çarpışma
        Rectangle karakterKutusu = { karakterX, karakterY, (float)karakterGenislik, (float)karakterYukseklik };
        for (int i = 0; i < MAX_DIKEN; i++) {
            if (dikenler[i].aktif) {
                dikenler[i].kalanSure -= dt;
                if (dikenler[i].kalanSure <= 0.0f) dikenler[i].aktif = false;

                Rectangle dikenKutu = { dikenler[i].x, dikenler[i].y, dikenler[i].genislik, dikenler[i].yukseklik };
                if (CheckCollisionRecs(karakterKutusu, dikenKutu)) {
                    mevcutCan -= 2;
                    sarsintiSuresi = 0.15f;
                    YaziEkle(karakterX, karakterY - 10, "-2", RED);
                }
            }
        }

        // Düşman Mermileri ve Parry
        for (int i = 0; i < MAX_DUSMAN_MERMI; i++) {
            if (dusmanMermiler[i].aktif) {
                dusmanMermiler[i].x += dusmanMermiler[i].hizX;
                if (dusmanMermiler[i].x < 0 || dusmanMermiler[i].x > 1600) dusmanMermiler[i].aktif = false;

                Rectangle dMermiKutusu = { dusmanMermiler[i].x, dusmanMermiler[i].y, 8, 5 };
                if (CheckCollisionRecs(dMermiKutusu, karakterKutusu)) {
                    if (parryAktif) {
                        dusmanMermiler[i].aktif = false;
                        beyazFlashSuresi = 0.08f;
                        mevcutCan = (mevcutCan + 20 > maxCan) ? maxCan : mevcutCan + 20;

                        GuvenliSesCal(sndParry);
                        YaziEkle(karakterX, karakterY - 25, "PARRY!", SKYBLUE);
                        YaziEkle(karakterX, karakterY - 10, "+20 HP", GREEN);
                        SokDalgasiYarat(karakterX + 20, karakterY + 20, 65.0f, SKYBLUE);
                        ParcacikPatlat(karakterX + 20, karakterY + 20, WHITE, 15, 4.0f);

                        for (int m = 0; m < MAX_OYUNCU_MERMI; m++) {
                            if (!oyuncuMermiler[m].aktif) {
                                oyuncuMermiler[m].x = karakterX + (karakterYon > 0 ? karakterGenislik : -10);
                                oyuncuMermiler[m].y = karakterY + (karakterYukseklik / 2.0f);
                                oyuncuMermiler[m].hizX = -dusmanMermiler[i].hizX * 1.8f; 
                                oyuncuMermiler[m].yansitildi = true;
                                oyuncuMermiler[m].aktif = true;
                                break;
                            }
                        }
                        sarsintiSuresi = 0.25f;
                    } 
                    else {
                        dusmanMermiler[i].aktif = false;
                        int mermiHasari = 15;
                        if (mevcutZirh > 0) {
                            mevcutZirh -= mermiHasari;
                            if (mevcutZirh < 0) {
                                mevcutCan += mevcutZirh;
                                mevcutZirh = 0;
                            }
                        } else {
                            mevcutCan -= mermiHasari;
                        }
                        sarsintiSuresi = 0.2f;
                        GuvenliSesCal(sndHit);
                        YaziEkle(karakterX, karakterY - 10, "-15", RED);
                    }
                }
            }
        }

        // Platform Çarpışması ve Slam
        yerdeMi = false; 
        for (int i = 0; i < platformSayisi; i++) {
            if (CheckCollisionRecs(karakterKutusu, platformlar[i]) && hizY >= 0) {
                karakterY = platformlar[i].y - karakterYukseklik; 

                if (slamYapiyorMusu) {
                    sarsintiSuresi = 0.5f; 
                    slamYapiyorMusu = false; 
                    GuvenliSesCal(sndSlam);

                    SokDalgasiYarat(karakterX + 20, platformlar[i].y, 90.0f, ORANGE);
                    ParcacikPatlat(karakterX + 20, platformlar[i].y, LIGHTGRAY, 20, 4.5f);

                    Rectangle slamAlani = { karakterX - 60, karakterY - 20, (float)karakterGenislik + 120, (float)karakterYukseklik + 20 };
                    for (int j = 0; j < dalgaDusmanSayisi; j++) {
                        if (dusmanlar[j].hayattaMi) {
                            Rectangle dusmanKutusu = { dusmanlar[j].x, dusmanlar[j].y, dusmanlar[j].genislik, dusmanlar[j].yukseklik };
                            if (CheckCollisionRecs(slamAlani, dusmanKutusu)) {
                                DusmanaHasarVer(j, oyuncuSlamHasari);
                            }
                        }
                    }
                }

                hizY = 0.0f;
                yerdeMi = true;
            }
        }

        // Temas Hasarı
        for (int i = 0; i < dalgaDusmanSayisi; i++) {
            if (!dusmanlar[i].hayattaMi) continue; 
            Rectangle dusmanKutusu = { dusmanlar[i].x, dusmanlar[i].y, dusmanlar[i].genislik, dusmanlar[i].yukseklik };

            if (CheckCollisionRecs(karakterKutusu, dusmanKutusu)) {
                if (hizY > 0 && karakterY + karakterYukseklik - 15 < dusmanKutusu.y) {
                    hizY = -8.0f;           
                    DusmanaHasarVer(i, 30);
                } 
                else {
                    if (dusmanlar[i].isBoss && dusmanlar[i].bossDurum == 1 && parryAktif) {
                        beyazFlashSuresi = 0.08f;
                        mevcutCan = (mevcutCan + 20 > maxCan) ? maxCan : mevcutCan + 20;
                        dusmanlar[i].bossDurum = 0; 
                        sarsintiSuresi = 0.35f;
                        GuvenliSesCal(sndParry);
                        YaziEkle(karakterX, karakterY - 25, "PARRY!", SKYBLUE);
                        SokDalgasiYarat(karakterX + 20, karakterY + 20, 80.0f, SKYBLUE);
                    } 
                    else {
                        int temasDarbe = (dusmanlar[i].distro == DISTRO_DEBIAN) ? 2 : 1;
                        if (mevcutZirh > 0) mevcutZirh -= temasDarbe;
                        else mevcutCan -= temasDarbe;
                    }
                }
            }
        }

        // Eşya Toplama
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (itemler[i].aktif) {
                Rectangle itemKutusu = { itemler[i].x, itemler[i].y, 20, 20 };
                if (CheckCollisionRecs(karakterKutusu, itemKutusu)) {
                    GuvenliSesCal(sndLoot);
                    if (itemler[i].tip == ITEM_CAN) {
                        mevcutCan = (mevcutCan + 30 > maxCan) ? maxCan : mevcutCan + 30;
                        YaziEkle(karakterX, karakterY - 15, "+30 CAN", GREEN);
                    } else if (itemler[i].tip == ITEM_MERMI) {
                        yedekMermi += itemler[i].miktar;
                        YaziEkle(karakterX, karakterY - 15, TextFormat("+%d MERMI", itemler[i].miktar), GOLD);
                    } else if (itemler[i].tip == ITEM_ZIRH) {
                        mevcutZirh = (mevcutZirh + 40 > maxZirh) ? maxZirh : mevcutZirh + 40;
                        YaziEkle(karakterX, karakterY - 15, "+40 ZIRH", SKYBLUE);
                    }
                    ParcacikPatlat(itemler[i].x + 10, itemler[i].y + 10, GOLD, 8, 2.0f);
                    itemler[i].aktif = false;
                }
            }
        }

        // Sarı Zafer Objesi
        if (zaferObjesiAktif) {
            if (CheckCollisionRecs(karakterKutusu, zaferObjesi)) {
                zaferObjesiAktif = false;
                GuvenliSesCal(sndLoot);
                if (enIyiSure < 0.0f || kosuSuresi < enIyiSure) enIyiSure = kosuSuresi;
                miniFightTekrar = 0; 
                IstatistikleriKaydet(miniFightTekrar, enIyiSure, enYuksekDalga);
                durum = OYUN_KAZANDIN;
                continue;
            }
        }

        // --- PARTİKÜLLERİ GÜNCELLE ---
        for (int i = 0; i < MAX_PARCACIK; i++) {
            if (parcaciklar[i].aktif) {
                parcaciklar[i].x += parcaciklar[i].hizX;
                parcaciklar[i].y += parcaciklar[i].hizY;
                parcaciklar[i].omur -= dt;
                if (parcaciklar[i].omur <= 0.0f) parcaciklar[i].aktif = false;
            }
        }

        // --- UÇAN YAZILARI GÜNCELLE ---
        for (int i = 0; i < MAX_YAZI; i++) {
            if (ucanYazilar[i].aktif) {
                ucanYazilar[i].y -= 35.0f * dt; // Yukarı süzülür
                ucanYazilar[i].omur -= dt;
                if (ucanYazilar[i].omur <= 0.0f) ucanYazilar[i].aktif = false;
            }
        }

        // --- ŞOK DALGALARINI GÜNCELLE ---
        for (int i = 0; i < MAX_SOK; i++) {
            if (sokDalgalari[i].aktif) {
                sokDalgalari[i].yaricap += 280.0f * dt;
                if (sokDalgalari[i].yaricap >= sokDalgalari[i].maxYaricap) {
                    sokDalgalari[i].aktif = false;
                }
            }
        }

        // --- YUMUŞATILMIŞ KAMERA TAKİBİ (SMOOTH LERP) ---
        Vector2 hedefKoordinat = { karakterX + (karakterGenislik / 2.0f), karakterY + (karakterYukseklik / 2.0f) };
        if (sarsintiSuresi > 0) {
            sarsintiSuresi -= dt;
            hedefKoordinat.x += GetRandomValue(-6, 6);
            hedefKoordinat.y += GetRandomValue(-6, 6);
        }
        // Lerp formülü: Mevcut konumdan hedefe %12 oranında yumuşak akış
        kamera.target.x += (hedefKoordinat.x - kamera.target.x) * 0.12f;
        kamera.target.y += (hedefKoordinat.y - kamera.target.y) * 0.12f;

        // ÇİZİM
        BeginDrawing();
        ClearBackground(BLACK); 

        BeginMode2D(kamera);
            DrawTexture(arkaplan, 0, 0, WHITE);
            DrawTexture(arkaplan, 800, 0, WHITE); 

            for (int i = 0; i < platformSayisi; i++) {
                DrawRectangleRec(platformlar[i], DARKGRAY);
            }

            for (int i = 0; i < MAX_DIKEN; i++) {
                if (dikenler[i].aktif) {
                    DrawTriangle(
                        (Vector2){ dikenler[i].x + (dikenler[i].genislik / 2.0f), dikenler[i].y },
                        (Vector2){ dikenler[i].x, dikenler[i].y + dikenler[i].yukseklik },
                        (Vector2){ dikenler[i].x + dikenler[i].genislik, dikenler[i].y + dikenler[i].yukseklik },
                        (bossFaz == 2) ? MAROON : RED
                    );
                }
            }

            for (int i = 0; i < MAX_ITEMS; i++) {
                if (itemler[i].aktif) {
                    if (itemler[i].tip == ITEM_CAN) {
                        DrawRectangle((int)itemler[i].x, (int)itemler[i].y, 18, 18, GREEN);
                        DrawText("+", (int)itemler[i].x + 4, (int)itemler[i].y + 1, 16, WHITE);
                    } else if (itemler[i].tip == ITEM_MERMI) {
                        DrawRectangle((int)itemler[i].x, (int)itemler[i].y, 22, 18, GOLD);
                        DrawText(TextFormat("+%d", itemler[i].miktar), (int)itemler[i].x + 2, (int)itemler[i].y + 3, 11, BLACK);
                    } else if (itemler[i].tip == ITEM_ZIRH) {
                        DrawRectangle((int)itemler[i].x, (int)itemler[i].y, 18, 18, SKYBLUE);
                        DrawText("Z", (int)itemler[i].x + 4, (int)itemler[i].y + 2, 14, WHITE);
                    }
                }
            }

            if (zaferObjesiAktif) {
                float dalgalanma = sinf(GetTime() * 6.0f) * 4.0f;
                DrawCircle((int)(zaferObjesi.x + 15), (int)(zaferObjesi.y + 15 + dalgalanma), 24 + dalgalanma, Fade(GOLD, 0.4f));
                DrawRectangleRounded((Rectangle){ zaferObjesi.x, zaferObjesi.y + dalgalanma, zaferObjesi.width, zaferObjesi.height }, 0.5f, 4, YELLOW);
                DrawRectangleRoundedLines((Rectangle){ zaferObjesi.x, zaferObjesi.y + dalgalanma, zaferObjesi.width, zaferObjesi.height }, 0.5f, 4, WHITE);
                DrawText("ZAFERI AL!", (int)zaferObjesi.x - 25, (int)zaferObjesi.y - 25 + (int)dalgalanma, 14, GOLD);
            }

            // Düşmanlar
            for (int i = 0; i < dalgaDusmanSayisi; i++) {
                if (dusmanlar[i].hayattaMi) {
                    Texture2D aktifTex = texMint;
                    Color fallbackRenk = GREEN;
                    const char* etiket = "MINT";

                    switch (dusmanlar[i].distro) {
                        case DISTRO_DEBIAN: aktifTex = texDebian; fallbackRenk = RED; etiket = "DEB"; break;
                        case DISTRO_FEDORA: aktifTex = texFedora; fallbackRenk = BLUE; etiket = "FED"; break;
                        case DISTRO_KALI:   aktifTex = texKali;   fallbackRenk = DARKBLUE; etiket = "KALI"; break;
                        case DISTRO_POPOS:  aktifTex = texPopos;  fallbackRenk = SKYBLUE; etiket = "POP"; break;
                        case DISTRO_ZORIN:  aktifTex = texZorin;  fallbackRenk = LIGHTGRAY; etiket = "ZOR"; break;
                        case DISTRO_UBUNTU: aktifTex = texUbuntu; fallbackRenk = ORANGE; etiket = "UBUNTU"; break;
                        default: break;
                    }

                    Rectangle destRec = { dusmanlar[i].x, dusmanlar[i].y, dusmanlar[i].genislik, dusmanlar[i].yukseklik };

                    if (aktifTex.id > 0) {
                        Rectangle srcRec = { 0, 0, (float)aktifTex.width, (float)aktifTex.height };
                        Color filtre = WHITE;
                        if (dusmanlar[i].isBoss) {
                            if (dusmanlar[i].bossParryAktif) filtre = GOLD;
                            else if (bossFaz == 2) filtre = RED;
                            else filtre = (dusmanlar[i].bossDurum == 1) ? VIOLET : PURPLE;
                        }
                        DrawTexturePro(aktifTex, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, filtre);
                    } else {
                        DrawRectangleRec(destRec, fallbackRenk);
                        DrawText(etiket, (int)dusmanlar[i].x + 4, (int)dusmanlar[i].y + 12, 11, WHITE);
                    }

                    if (dusmanlar[i].isBoss && dusmanlar[i].bossParryAktif) {
                        DrawCircleLines((int)(dusmanlar[i].x + dusmanlar[i].genislik / 2), (int)(dusmanlar[i].y + dusmanlar[i].yukseklik / 2), (float)dusmanlar[i].genislik * 0.7f, GOLD);
                    }

                    if (!dusmanlar[i].isBoss) {
                        float canOrani = (float)dusmanlar[i].can / (float)dusmanlar[i].maxCan;
                        DrawRectangle((int)dusmanlar[i].x, (int)dusmanlar[i].y - 8, (int)dusmanlar[i].genislik, 4, DARKGRAY);
                        DrawRectangle((int)dusmanlar[i].x, (int)dusmanlar[i].y - 8, (int)(dusmanlar[i].genislik * canOrani), 4, RED);
                    }
                }
            }

            // Mermiler
            for (int i = 0; i < MAX_OYUNCU_MERMI; i++) {
                if (oyuncuMermiler[i].aktif) {
                    DrawRectangle((int)oyuncuMermiler[i].x, (int)oyuncuMermiler[i].y, 8, 4, oyuncuMermiler[i].yansitildi ? SKYBLUE : YELLOW);
                }
            }

            for (int i = 0; i < MAX_DUSMAN_MERMI; i++) {
                if (dusmanMermiler[i].aktif) {
                    int mG = (bossFaz == 2) ? 10 : 8;
                    int mY = (bossFaz == 2) ? 6 : 5;
                    DrawRectangle((int)dusmanMermiler[i].x, (int)dusmanMermiler[i].y, mG, mY, (bossFaz == 2) ? ORANGE : RED);
                }
            }

            if (yumrukEfektSuresi > 0) {
                DrawRectangle(
                    karakterYon > 0 ? (int)karakterX + karakterGenislik : (int)karakterX - 35, 
                    (int)karakterY + 5, 
                    35, 
                    karakterYukseklik - 10, 
                    ORANGE
                );
            }

            // Karakter
            DrawTexture(karakter, (int)karakterX, (int)karakterY, dashAtiyorMu ? SKYBLUE : WHITE);

            if (parryAktif) {
                DrawCircleLines((int)(karakterX + karakterGenislik / 2.0f), (int)(karakterY + karakterYukseklik / 2.0f), 28, SKYBLUE);
                DrawCircleLines((int)(karakterX + karakterGenislik / 2.0f), (int)(karakterY + karakterYukseklik / 2.0f), 30, WHITE);
            }

            // --- PARTİKÜLLERİ ÇİZ ---
            for (int i = 0; i < MAX_PARCACIK; i++) {
                if (parcaciklar[i].aktif) {
                    float oran = parcaciklar[i].omur / parcaciklar[i].maxOmur;
                    Color solukRenk = Fade(parcaciklar[i].renk, oran);
                    DrawRectangle((int)parcaciklar[i].x, (int)parcaciklar[i].y, (int)(parcaciklar[i].boyut * oran), (int)(parcaciklar[i].boyut * oran), solukRenk);
                }
            }

            // --- ŞOK DALGALARINI ÇİZ ---
            for (int i = 0; i < MAX_SOK; i++) {
                if (sokDalgalari[i].aktif) {
                    float oran = 1.0f - (sokDalgalari[i].yaricap / sokDalgalari[i].maxYaricap);
                    DrawCircleLines((int)sokDalgalari[i].x, (int)sokDalgalari[i].y, sokDalgalari[i].yaricap, Fade(sokDalgalari[i].renk, oran));
                }
            }

            // --- UÇAN YAZILARI ÇİZ ---
            for (int i = 0; i < MAX_YAZI; i++) {
                if (ucanYazilar[i].aktif) {
                    float oran = ucanYazilar[i].omur / 0.85f;
                    DrawText(ucanYazilar[i].metin, (int)ucanYazilar[i].x, (int)ucanYazilar[i].y, 14, Fade(ucanYazilar[i].renk, oran));
                }
            }

        EndMode2D(); 

        // UI
        Color canRengi = (mevcutCan > 60) ? GREEN : ((mevcutCan >= 30) ? YELLOW : RED);
        DrawRectangle(20, 18, maxCan * 2, 14, DARKGRAY); 
        DrawRectangle(20, 18, mevcutCan * 2, 14, canRengi);   
        DrawRectangleLines(20, 18, maxCan * 2, 14, WHITE); 
        DrawText(TextFormat("CAN: %d/%d", mevcutCan, maxCan), 25, 19, 11, WHITE);

        DrawRectangle(20, 36, maxZirh * 2, 8, DARKGRAY); 
        DrawRectangle(20, 36, mevcutZirh * 2, 8, SKYBLUE);   
        DrawRectangleLines(20, 36, maxZirh * 2, 8, WHITE); 
        DrawText(TextFormat("ZIRH: %d/%d", mevcutZirh, maxZirh), maxZirh * 2 + 30, 35, 11, SKYBLUE);

        int ziplamaY = 48;
        for (int i = 0; i < maxZiplamaHakki; i++) {
            bool dolu = i < kalanHak;
            Color kapsulRengi = dolu ? SKYBLUE : DARKGRAY;
            Rectangle kapsulRec = { (float)(baslangicX + i * (kapsulGenislik + bosluk)), (float)ziplamaY, (float)kapsulGenislik, (float)kapsulYukseklik };
            DrawRectangleRounded(kapsulRec, 0.5f, 4, kapsulRengi);
            DrawRectangleRoundedLines(kapsulRec, 0.5f, 4, WHITE); 
        }

        int dashY = 62;
        for (int i = 0; i < maxDashHakki; i++) {
            bool dolu = i < kalanDash;
            Color dashKapsulRengi = dolu ? GOLD : DARKGRAY;
            Rectangle dashRec = { (float)(baslangicX + i * (kapsulGenislik + bosluk)), (float)dashY, (float)kapsulGenislik, (float)kapsulYukseklik };
            DrawRectangleRounded(dashRec, 0.5f, 4, dashKapsulRengi);
            DrawRectangleRoundedLines(dashRec, 0.5f, 4, WHITE); 
        }

        if (reloadYapiyor) {
            DrawText("DOLDURULUYOR...", 20, 78, 14, RED);
        } else {
            DrawText(TextFormat("SARJOR: %d / %d  |  YEDEK: %d (R: Doldur)", sarjorMermi, sarjorKapasite, yedekMermi), 20, 78, 13, RAYWHITE);
        }

        int dkk = (int)kosuSuresi / 60;
        int snn = (int)kosuSuresi % 60;
        int sal = (int)((kosuSuresi - (int)kosuSuresi) * 100);

        if (seciliMod == MOD_MINI_FIGHT) {
            DrawText(TextFormat("SURE: %02d:%02d.%02d", dkk, snn, sal), 580, 20, 16, GOLD);
            DrawText(TextFormat("TEKRAR: %d", miniFightTekrar), 680, 42, 14, RED);
        } else {
            DrawText(TextFormat("DALGA: %d  (REKOR: %d)", dalga, enYuksekDalga), 530, 20, 16, YELLOW);
            DrawText(TextFormat("SURE: %02d:%02d", dkk, snn), 680, 42, 14, LIGHTGRAY);
        }

        // Boss Can Barı
        if (seciliMod == MOD_MINI_FIGHT && dalga == 2 && dusmanlar[0].hayattaMi) {
            float bossOran = (float)dusmanlar[0].can / (float)dusmanlar[0].maxCan;
            Color barRengi = (bossFaz == 2) ? RED : ORANGE;
            const char* bossBaslik = (bossFaz == 2) ? "UBUNTU BOSS - FAZ 2 (SNAP KUDURMASI)" : "UBUNTU BOSS - FAZ 1";

            DrawRectangle(250, 20, 300, 16, DARKGRAY);
            DrawRectangle(250, 20, (int)(300 * bossOran), 16, barRengi);
            DrawRectangleLines(250, 20, 300, 16, WHITE);
            DrawText(bossBaslik, 280, 22, 12, WHITE);

            if (dusmanlar[0].bossParryAktif) {
                DrawText("BOSS PARI DURUSUNDA!", 330, 42, 12, GOLD);
            }
        }

        if (faz2BildirimSayaci > 0.0f) {
            DrawText("ADRENALIN PATLAMASI! (CAN, ZIRH & MERMI TAZELENDI)", 150, 240, 20, GREEN);
            DrawText("DIKKAT: UBUNTU BOSS 2. FAZA GECTI!", 220, 275, 26, RED);
        }

        DrawText("F: Parry | SPACE: Ates | ENTER: Yumruk | SHIFT: Dash | ASAGI: Slam", 20, 575, 12, LIGHTGRAY);

        if (beyazFlashSuresi > 0) {
            DrawRectangle(0, 0, 800, 600, Fade(RAYWHITE, 0.85f));
        }

        EndDrawing();
    }

    // Sesleri ve Dokuları Temizle
    if (sndAtes.stream.buffer != NULL)  UnloadSound(sndAtes);
    if (sndParry.stream.buffer != NULL) UnloadSound(sndParry);
    if (sndHit.stream.buffer != NULL)   UnloadSound(sndHit);
    if (sndDash.stream.buffer != NULL)  UnloadSound(sndDash);
    if (sndSlam.stream.buffer != NULL)  UnloadSound(sndSlam);
    if (sndLoot.stream.buffer != NULL)  UnloadSound(sndLoot);
    CloseAudioDevice();

    UnloadTexture(arkaplan);
    UnloadTexture(karakter);
    UnloadTexture(texMint);
    UnloadTexture(texDebian);
    UnloadTexture(texFedora);
    UnloadTexture(texKali);
    UnloadTexture(texPopos);
    UnloadTexture(texZorin);
    UnloadTexture(texUbuntu);

    CloseWindow(); 
    return 0;
}