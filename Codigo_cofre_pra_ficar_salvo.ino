#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Teclado
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {  
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
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

// Lista de até 5 UIDs cadastrados
String tagsCadastradas[MAX_TAGS] = {"", "", "", "", ""};

bool cofreAberto = false;

byte rowPins[ROWS] = {12, 14, 27, 26};
byte colPins[COLS] = {25, 33, 32, 35};


Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variáveis
const String senhapadrao = "000000";
String senhaatual = senhapadrao;
String senhadigitada;
Servo servo;
int pinoalarme = 9; // pino PWM para alarme
int bloqueios = 0;
int senhaserradas = 3;
bool estadoporta = false;

void setup() {
  lcd.init();          // Inicializa I2C
  lcd.backlight();     // Liga backlight
  pinMode(pinoalarme, OUTPUT);
  servo.attach(10);    // Servo no pino 10
  servo.write(0); 
  lcd.print("Digite a senha:");
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
  char tecla = keypad.getKey();
  
  if (tecla) {
    if (tecla == '#') {
      if (senhadigitada == senhaatual) {
        abrirporta();
        senhaserradas = 3;
      } else {
        errosdesenha();
      }
      senhadigitada = "";
    }
    else if (tecla == 'A') {
      mudarsenha();
    }
    else if (tecla == 'B') {
      fecharporta();
    }
    else if (tecla == '*') {
      if (senhadigitada.length() > 0) {
        senhadigitada.remove(senhadigitada.length() - 1);
        updateDisplay();
      }
    }
    else if (tecla >= '0' && tecla <= '9') { 
      senhadigitada += tecla;
      updateDisplay();
    }
  }
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

void abrirporta() {
  lcd.clear();
  lcd.print("Porta Aberta");
  servo.write(180);
  estadoporta = true;
  delay(5000);
  lcd.clear();
  lcd.print("Pressione B");
  lcd.setCursor(0, 1);
  lcd.print("para fechar");
}

void fecharporta() {
  if (estadoporta) {
    lcd.clear();
    lcd.print("Fechando porta");
    servo.write(0);
    estadoporta = false;
    delay(2000);
    resetDisplay();
  } else {
    lcd.clear();
    lcd.print("Porta ja fechada!");
    delay(2000);
    resetDisplay();
  }
}

void errosdesenha() {
  senhaserradas--;
  if (senhaserradas <= 0) {
    triggerAlarm();
  } else {
    lcd.clear();
    lcd.print("Senha Incorreta!");
    lcd.setCursor(0, 1);
    lcd.print("Tentativas: " + String(senhaserradas));
    delay(2000);
    resetDisplay();
  }
}

void mudarsenha() {
  lcd.clear();
  lcd.print("Senha Atual:");
  String tempPassword = "";
  
  while (true) {
    char authKey = keypad.getKey();
    if (authKey) {
      if (authKey == '#') {
        if (tempPassword == senhaatual) {
          senhaserradas = 3;
          break;
        } else {
          lcd.clear();
          lcd.print("Senha Errada!");
          delay(2000);
          resetDisplay();
          return;
        }
      } else if (authKey == '*') {
        if (tempPassword.length() > 0) {
          tempPassword.remove(tempPassword.length() - 1);
        }
      } else if (authKey >= '0' && authKey <= '9') {
        tempPassword += authKey;
      }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      for (int i = 0; i < tempPassword.length(); i++) lcd.print('*');
    }
  }

  lcd.clear();
  lcd.print("Nova Senha:");
  String newPassword = "";
  
  while (true) {
    char newKey = keypad.getKey();
    if (newKey) {
      if (newKey == '#') {
        if (newPassword.length() >= 6) {
          senhaatual = newPassword;
          lcd.clear();
          lcd.print("Senha Alterada!");
          delay(2000);
          resetDisplay();
          return;
        } else {
          lcd.clear();
          lcd.print("Min. 6 digitos!");
          delay(2000);
          resetDisplay();
          return;
        }
      } else if (newKey == '*') {
        if (newPassword.length() > 0) {
          newPassword.remove(newPassword.length() - 1);
        }
      } else if (newKey >= '0' && newKey <= '9') {
        newPassword += newKey;
      }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      for (int i = 0; i < newPassword.length(); i++) lcd.print('*');
    }
  }
}

void triggerAlarm() {
  bloqueios++;
  lcd.clear();
  lcd.print("Sistema");
  lcd.setCursor(0, 1);
  lcd.print("Bloqueado!");
  
  unsigned long startTime = millis();
  
  while (millis() - startTime < bloqueios * 180000UL) { 
    if ((millis() - startTime) % 1000 < 500) {
      digitalWrite(pinoalarme, HIGH);
    } else {
      digitalWrite(pinoalarme, LOW);
    }
  }
  
  digitalWrite(pinoalarme, LOW);
  senhaserradas = 3;
  resetDisplay();
}

void resetDisplay() {
  lcd.clear();
  lcd.print("Digite a senha:");
}

void updateDisplay() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  for (int i = 0; i < senhadigitada.length(); i++) {
    lcd.print('*');
  }
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
