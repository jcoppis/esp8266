#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22

ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

char ssid[20];
char pass[20];

int ssid_tamano = 0;
int pass_tamano = 0;

bool conectado = false;
bool enviarDatos = false;
float h = 0.0, t = 0.0, hic = 0.0;
const int nroEstacion = 13;
const char* nombreEstacion = "estacionMed";
const char* pagina = "www.labea.criba.edu.ar";

unsigned long tiempo = 0;
unsigned long intervaloMed = 5000;
unsigned long intervaloConexion = 600 * 1000; // diez min

void principal() {

  String conexion = (conectado) ? "conectado" : "no conectado";
  
  String json = "{";
  json += "\"nombre\":" + String(nombreEstacion);
  json += ", \"numero\":" + String(nroEstacion);
  json += "\"conexion\":\"" + conexion + "\"";
  json += ", \"WiFi\":\"" + String(ssid) + "\"";
  json += ", \"IP\":\"" + WiFi.localIP().toString() + "\"";
  json += ", \"enviarDatos\":" + String(enviarDatos);
  json += "}";
  server.send(200, "text/json", json);
  Serial.println("Enviando datos: " + json);
  json = String();
    
  String pral = "<!DOCTYPE html>"
                "<html lang='es'>"
                "<head>"
                "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>"
                "<title>" + String(nombreEstacion) + "</title>"
                "<style type='text/css'> body,td,th { color: #036; } body { background-color: #999; } </style>"
                "</head>"
                "<body>"
                "<h1>" + String(nombreEstacion) + " " + String(nroEstacion) + "</h1><br>";

  if (!conectado) { //WiFi.status() != WL_CONNECTED) {
    pral += "<h1>WIFI CONF</h1><br>";

    String selector = "";

    Serial.println("scan start");

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no se encontraron redes WiFi");
      pral += "<p>no se encontraron redes WiFi</p>"
              "</body>"
              "</html>";
    }
    else
    {
      Serial.print(n);
      Serial.println(" redes WiFi encontradas");

      selector = "<select name='ssid'>";
      for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        selector += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>" ;
        //delay(10);
      }
      selector += "</select>";

      pral += "<form action='config' method='POST' target='pantalla'>"
              "<fieldset align='left' style='border-style:solid; border-color:#336666; width:200px; height:180px; padding:10px; margin: 5px;'>"
              "<legend><strong>Configurar WI-FI</strong></legend>"
              "SSID: <br> " + selector + "<br><br>"
              "PASSWORD: <br> <input name='pass' type='password' /> <br><br>"
              "<input type='submit' value='Conectar' />"
              "</fieldset>"
              "</form>"
              "<iframe id='pantalla' name='pantalla' src='' width=800px height=400px frameborder='0' scrolling='no'></iframe>"
              "</body>"
              "</html>";
    }
  }
  else {
    pral += "<h2>Ya esta conectado con: " + String(ssid) + "</h2><br>"
            "<h2>En el IP: " + WiFi.localIP().toString() + "</h2><br>"
            "<a href='reconf'><button>Reconfigurar</button></a>"
            "</body>"
            "</html>";
  }
  server.send(200, "text/html", pral);
}

void enviarValores(float temp, float hum, int nroEstacion) {
  String datosJSON = "{\"temperatura\":" + (String)temp + ", \"humedad\":" + (String)hum + ", \"estacion\":" + (String)nroEstacion + "}"; //JSON data
  String paqueteJSON = "med=" + datosJSON;
  String length = String(paqueteJSON.length());

  // If we get a proper connection to the Ubidots API
  if (client.connect(pagina, 80))
  {
    Serial.println("Conectado a " + (String)pagina);
    delay(100);

    // Construct the POST request that we'd like to issue
    client.println("POST /guardar.php HTTP/1.1");
    // We also use the Serial terminal to show how the POST request looks like
    Serial.println("POST /estacionMeteorologica/api/123/medicion HTTP/1.1");
    // Specify the contect type so it matches the format of the data (JSON)
    client.println("Content-Type: application/x-www-form-urlencoded");
    Serial.println("Content-Type: application/x-www-form-urlencoded");
    // Specify the content length
    client.println("Content-Length: " + length);
    Serial.println("Content-Length: " + length);
    // Specify the host
    client.println("Host: " + (String)pagina + "\n");
    Serial.println("Host: " + (String)pagina + "\n");
    // Send the actual data
    client.print(paqueteJSON);
    Serial.println(paqueteJSON);
  }
  else
  {
    // If we can't establish a connection to the server:
    Serial.println("Fallo la conexion con " + (String)pagina);
  }

  // respuesta del server al que se le envian los datos
  while (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }
  Serial.println();

  // Done with this iteration, close the connection.
  if (client.connected())
  {
    Serial.println("Desconectado de " + (String)pagina);
    client.stop();
  }
}

String arregla_simbolos(String a) {
  a.replace("%C3%A1", "á");
  a.replace("%C3%A9", "é");
  a.replace("%C3%A", "i");
  a.replace("%C3%B3", "ó");
  a.replace("%C3%BA", "ú");
  a.replace("%21", "!");
  a.replace("%23", "#");
  a.replace("%24", "$");
  a.replace("%25", "%");
  a.replace("%26", "&");
  a.replace("%27", "/");
  a.replace("%28", "(");
  a.replace("%29", ")");
  a.replace("%3D", "=");
  a.replace("%3F", "?");
  a.replace("%27", "'");
  a.replace("%C2%BF", "¿");
  a.replace("%C2%A1", "¡");
  a.replace("%C3%B1", "ñ");
  a.replace("%C3%91", "Ñ");
  a.replace("+", " ");
  a.replace("%2B", "+");
  a.replace("%22", "\"");
  return a;
}

//*******  G R A B A R  EN LA  E E P R O M  ***********
void grabaString(int addr, String a) {
  int tamano = (a.length() + 1);
  char inchar[20];    //'30' Tamaño maximo del string
  a.toCharArray(inchar, tamano);
  EEPROM.write(addr, tamano);
  for (int i = 0; i < tamano; i++) {
    addr++;
    EEPROM.write(addr, inchar[i]);
  }
  EEPROM.commit();
}

//*******  L E E R   EN LA  E E P R O M    **************
String leeString(int addr) {
  String nuevoString;
  int valor;
  int tamano = EEPROM.read(addr);
  for (int i = 0; i < tamano; i++) {
    addr++;
    valor = EEPROM.read(addr);
    nuevoString += (char)valor;
  }
  return nuevoString;
}

void reconf() {
  conectado = false;
  principal();
}

//**** CONFIGURACION WIFI  *******
void wifi_conf() {
  int cuenta = 0;

  String getssid = server.arg("ssid"); //Recibimos los valores que envia por GET el formulario web
  String getpass = server.arg("pass");

  getssid = arregla_simbolos(getssid); //Reemplazamos los simbolos que aparecen con UTF8 por el simbolo correcto
  getpass = arregla_simbolos(getpass);

  ssid_tamano = getssid.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
  pass_tamano = getpass.length() + 1;

  getssid.toCharArray(ssid, ssid_tamano); //Transformamos el string en un char array ya que es lo que nos pide WIFI.begin()
  getpass.toCharArray(pass, pass_tamano);

  Serial.println(ssid);     //para depuracion
  Serial.println(pass);

  if (!conectado) {
    WiFi.begin(ssid, pass);     //Intentamos conectar
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      cuenta++;
      if (cuenta > 20) { //10 segs
        conectado = false;
        grabaString(70, "noconfigurado");
        Serial.println("No se pudo conectar");
        server.send(200, "text/html", String("<h2>No se pudo realizar la conexion, no se guardaron los datos.</h2>"));
        return;
      }
    }
    conectado = true;
    grabaString(43, "configurado");
    grabaString(1, getssid);
    grabaString(22, getpass);
    grabaString(64, String(intervaloMed, DEC));
    Serial.println(WiFi.localIP());
    server.send(200, "text/html", String("<h2>Conexion exitosa a: " + getssid + "<br>El pass ingresado es: " + getpass + "<br>Datos correctamente guardados.</h2>"));
  }
  else {
    server.send(200, "text/html", String("<h2>Ya se encuentra conectado con: " + getssid + "</h2>"));
  }
}

//*********  INTENTO DE CONEXION   *********************
void intento_conexion() {
  if (leeString(43).equals("configurado")) {
    String ssid_leido = leeString(1);      //leemos ssid y password
    String pass_leido = leeString(22);
    intervaloMed = leeString(64).toInt();

    Serial.println(ssid_leido);  //Para depuracion
    Serial.println(pass_leido);

    ssid_tamano = ssid_leido.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
    pass_tamano = pass_leido.length() + 1;

    ssid_leido.toCharArray(ssid, ssid_tamano); //Transf. el String en un char array ya que es lo que nos pide WiFi.begin()
    pass_leido.toCharArray(pass, pass_tamano);

    int cuenta = 0;
    WiFi.begin(ssid, pass);      //Intentamos conectar
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      cuenta++;
      if (cuenta > 20) {
        conectado = false;
        Serial.println("Fallo al conectar");
        return;
      }
    }
  }
  else {
    Serial.println("La estacion " + String(nroEstacion) + " no esta configurada");
  }
  if (WiFi.status() == WL_CONNECTED) {
    conectado = true;

    Serial.print("Conexion exitosa a: ");
    Serial.println(ssid);
    Serial.print("Direccion IP: ");
    Serial.println(WiFi.localIP());
  }
}

//*****  S E T U P  **************
void setup() {
  Serial.begin(115200);
  EEPROM.begin(4096);
  WiFi.softAP(nombreEstacion);

  server.on("/", principal);

  server.on("/datos", HTTP_GET, []() {
    String json = "{";
    //json += "\"voltaje\":"+String(ESP.getVcc()); //ADC_MODE(ADC_VCC); no anda
    json += "\"temperatura\":" + String(t);
    json += ", \"humedad\":" + String(h);
    json += "}";
    server.send(200, "text/json", json);
    Serial.println("Enviando datos: " + json);
    json = String();
  });

  server.on("/conexion", HTTP_GET, []() {
    String conexion = (conectado) ? "conectado" : "no conectado";
    String json = "{";
    json += "\"conexion\":\"" + conexion + "\"";
    json += ", \"WiFi\":\"" + String(ssid) + "\"";
    json += ", \"IP\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    server.send(200, "text/json", json);
    Serial.println("Enviando datos: " + json);
    Serial.println(WiFi.localIP());
    json = String();
  });

  server.on("/intervaloMed", HTTP_GET, []() {
    intervaloMed = server.arg("intervalo").toInt();
    grabaString(64, String(intervaloMed, DEC));
    server.send(200, "text/plain", "intervalo=" + String(intervaloMed, DEC));
    Serial.println("intervaloMed= " + String(intervaloMed));
  });

  server.on("/enviardatos/0", HTTP_GET, []() {
    enviarDatos = false;
    server.send(200, "text/plain", "Se cancelo el envio de datos");
    Serial.println("enviarDatos=false");
  });

  server.on("/enviardatos/1", HTTP_GET, []() {
    enviarDatos = true;
    server.send(200, "text/plain", "Se habilito el envio de datos");
    Serial.println("enviarDatos=true");
  });

  server.on("/config", wifi_conf);
  server.on("/reconf", reconf);

  dht.begin();
  server.begin();
  Serial.println();
  Serial.println("Webserver iniciado");

  //Serial.println(ESP.getFlashChipRealSize());
  //Serial.println(lee(70));
  //Serial.println(lee(1));
  //Serial.println(lee(30));

  intento_conexion();
}


//*****   L O O P   **************
void loop() {
  server.handleClient();

  tiempo = millis();
  if (tiempo % intervaloMed == 0) {
    do {
      h = dht.readHumidity();
      t = dht.readTemperature();
    } while (isnan(h) || isnan(t));

    hic = dht.computeHeatIndex(t, h, false);

    Serial.println("Temp: " + (String)t + "*C,\t Hum: " + (String)h + "%,\t HI: " + (String)hic + "*C");
  }
  if (enviarDatos && conectado && tiempo % intervaloConexion == 0) {
    enviarValores(t, h, nroEstacion);
    //WiFi.forceSleepBegin();
    //WiFi.forceSleepWake();
    //delay(100)
    //VER hay que hacer una funcion para reconectar como si se reseteara todo
    //VER ESP.deepSleep(sendingPeriod*1000000);
  }
}
