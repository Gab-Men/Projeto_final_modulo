#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <Arduino.h>

// Definição dos pinos
#define SS_PIN 10
#define RST_PIN 9
#define SERVO_PIN 8
#define BUZZER 4
#define BOTAO_CADASTRO 7

// Definição dos pinos do LCD sem I2C
#define RS 12
#define EN 11
#define D4 5
#define D5 4
#define D6 3
#define D7 2

#define MAX_TAGS 5 

// Inicialização dos objetos
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo servo;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Lista de até 5 UIDs cadastrados
String tagsCadastradas[MAX_TAGS] = {"", "", "", "", ""};

int proximaPosicao = 0;

bool cofreAberto = false;

void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    servo.attach(SERVO_PIN);
    pinMode(BUZZER, OUTPUT);
    pinMode(BOTAO_CADASTRO, OUTPUT);
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Cofre Fechado");  
    servo.write(0);
    Serial.println("Aproxime o cartão para acesso...");
}

void loop() {
    if (digitalRead(BOTAO_CADASTRO) == HIGH) {
        cadastrar();
    }

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    String conteudo = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    conteudo.toUpperCase();
    
    Serial.print("Cartão lido: ");
    Serial.println(conteudo);

    if (verificarAcesso(conteudo)) {
        if (!cofreAberto) {
            abrirCofre();
        } else {
            fecharCofre();
        }
    } else {
        acessoNegado();
    }
    
    delay(2000);
    mfrc522.PICC_HaltA();
}

bool verificarAcesso(String uid) {
    for (int i = 0; i < 2; i++) {
        if (tagsCadastradas[i].equals(uid)) {
            return true;
        }
    }
    return false;
}

void cadastrar() {
    if (proximaPosicao >= MAX_TAGS) {  // Verifica se já atingiu o limite
        Serial.println("Limite de TAGs cadastradas atingido!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Limite atingido!");
        delay(2000);
        return;
    }

    Serial.println("Entrando no modo de cadastro...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cadastro de TAGs");

    Serial.print("Aproxime a TAG "); Serial.println(proximaPosicao + 1);
    lcd.setCursor(0, 1);
    lcd.print("Aproxime a TAG...");

    while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(100);
    }

    String novoUID = "";
    for (byte j = 0; j < mfrc522.uid.size; j++) {
        novoUID.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
        novoUID.concat(String(mfrc522.uid.uidByte[j], HEX));
    }
    novoUID.toUpperCase();

    // Verifica se a TAG já foi cadastrada
    for (int k = 0; k < proximaPosicao; k++) {
        if (tagsCadastradas[k].equals(novoUID)) {
            Serial.println("Esta TAG já está cadastrada!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("TAG Já Cadastrada!");
            delay(2000);
            return;
        }
    }

    // Cadastra a nova TAG
    tagsCadastradas[proximaPosicao] = novoUID;
    Serial.print("TAG cadastrada com sucesso: "); Serial.println(novoUID);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TAG Cadastrada!");

    proximaPosicao++;  // Avança a posição para o próximo cadastro
    delay(2000);
    
    Serial.println("Cadastro concluído!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cadastro Finalizado");
    delay(2000);
}

void abrirCofre() {
    Serial.println("Acesso permitido! Cofre aberto.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cofre Aberto!");
    servo.write(180);
    cofreAberto = true;
    delay(3000);
    fecharCofre();
}

void fecharCofre() {
    Serial.println("Fechando o cofre...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cofre Fechado");
    
    servo.write(0);
    cofreAberto = false;
}

void acessoNegado() {
    Serial.println("Acesso negado! TAG inválida.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TAG Inválida");
    digitalWrite(BUZZER, HIGH);
    delay(2000);
    digitalWrite(BUZZER, LOW);
    
}
