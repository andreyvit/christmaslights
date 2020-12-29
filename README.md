# Светодиодная гирлянда на базе ESP8266 или Arduino

Мы используем ESP8266 (с тулчейном Arduino) или же сам Arduino, чтобы реализовать прикольные эффекты для новогодней елки.

На Arduino собрать схему чуточку проще (при использовании 5-вольтовых светодиодных лент), тогда как на ESP8266 можно добавить Wi-Fi-интерфейс.

Компоненты:

* [светодиодная лента на базе 5050, вариант 1](https://ru.aliexpress.com/item/1M-2M-4M-5M-WS2812B-5V-RGB-Addressble-LED-Strip-Black-White-PCB-30-60-144/32461422496.html?spm=a2g0s.9042311.0.0.mrdQ9E)
* [светодиодная лента на базе 5050, вариант 2](https://ru.aliexpress.com/item/1m-5m-WS2812B-30-60-144-leds-m-Smartled-pixel-RGB-individually-addressable-led-strip/32807365734.html?spm=a2g0s.9042311.0.0.5n4cZB)
* [светодиоды для елки](https://ru.aliexpress.com/item/Wholesale-100pcs-9mm-White-Wire-WS2811-IC-Led-pixel-Module-string-Non-waterproof-IP30-RGB-Dream/32240898788.html?spm=a2g0s.9042311.0.0.mrdQ9E)
* [блок питания на 20 ампер](https://ru.aliexpress.com/item/IMC-Hot-AC-110-220V-DC-5V-20A-100W-Power-Supply-Driver-for-LED-Strip-Light/32633658736.html?spm=a2g0s.9042311.0.0.mrdQ9E)
* [модули ESP8266 примерно такие](https://ru.aliexpress.com/item/5pcs-Wireless-module-NodeMcu-Lua-WIFI-Internet-of-Things-development-board-based-ESP8266-CP2102-with-pcb/32723715227.html?spm=a2g0s.9042311.0.0.LaNnBG), искать по словам NodeMCU V3
* Arduino пойдет любой из традиционных 5-вольтовых, например, Uno, Mega


## Структура проекта

* в папке [`Christmas Lights`](Christmas Lights) — симулятор эффектов в виде приложения для iOS (сейчас сломан, скорее всего)

* в папке [`ChristmasLightsController`](ChristmasLightsController) — Arduino-проект, основной код см. в [`ChristmasLightsController.ino`](ChristmasLightsController/ChristmasLightsController.ino).
