Ceci est la partie embarquée des nouvelles plantoids,

    fonctionne sur ESP32, le olimex esp32-poe est la cible pour ce projet
    redirige l'audio du micro I2S vers un serveur websokets
    redirige l'audio du serveur vers un ampli embarqué en I2S
    dans notre cas d'application, nous avons besoin d'un half duplex seuleument (le micro est eteind pendant la diffusion sur les hauts parleurs)
    le serveur peut trigger des changements d'effets sur les leds (ws2812, librairie fastLed)
    le serveur peut trigger une activation du mode micro, du mode diffusion, ou du mode veille
    le montage de base doit consommer suffisament peut pour fonctionner en poe sur le olimex esp32-poe
    le montage doit pouvoir etre alimenté de facon externe pour permettre plus de leds ou un ampli audio plus puissant sur certaines plantoides plus grandes

à faire:

    nétoyage du code, 
    stocker les constantes de la plante dans le systeme de fichier pour les éditer via l'interface du wifimanager

COTé MATERIEL:

    board utilisée pour les essais initiaux : esp32 dev kit de base

    ref micro: inmp441

    ref de l'ampli utilisé: Max98357

    nombre de leds max envisagé: 25 (ws2812)

    à faire:

    prise en charge de capteurs et d'actionneurs (touch sensors,servomoteurs) au niveau de l'esp (meme si la partie moteur reste limité avec le nombre de watts maxi en mode poe),pins à détéerminer

    pcb, schema de cabalge

    boitier, design, impression 3d

    PINOUT: 
    
    // we define the button pin for wifimanager
    #define TRIGGER_PIN 34                // !!!! 34 for olimex ,33 esp32 dev kit
    
    // AMP I2S CONNECTIONS
    #define I2S_DOUT 13
    #define I2S_BCLK 14  // this should be the same as I2S_SCK for the mic
    #define I2S_LRC 15   // this should be the same as I2S_WS for the mic

    // MIC I2S CONNECTIONS
    #define I2S_SD 2
    #define I2S_WS 15
    #define I2S_SCK 14

    // Information about the LED strip itself
    #define LED_PIN 4