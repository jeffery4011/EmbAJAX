#include <EmbAJAX.h>
#include <EmbAJAXValidatingTextInput.h> // Fancier text input in a separate header file
#include <EmbAJAXScriptedSpan.h>

#define STASSID "Robots2" // name of router
#define STAPSK  "205207518asd" // pwd for router
const char* ssid = STASSID;
const char* password = STAPSK;

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDriverWebServerClass is a
// convenience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

#define BUFLEN 16
#define A_out1 4
#define A_out2 5
#define B_out1 12
#define B_out2 13
int fix_value = 100;
int range0 = 1100;
int sensorValue1 = 0;
int sensorValue2 = 0;

// Define the main elements of interest as variables, so we can access to them later in our sketch.
EmbAJAXMutableSpan sensor_1("sensor_1");
char sensor_1_buf[BUFLEN];

EmbAJAXMutableSpan sensor_2("sensor_2");
char sensor_2_buf[BUFLEN];

const char* radio_opts[] = {"stop", "spin", "ahead", "astern", "user"};
EmbAJAXRadioGroup<5> radio("radio", radio_opts);
EmbAJAXMutableSpan radio_d("radio_d");

EmbAJAXOptionSelect<5> optionselect("optionselect", radio_opts);
EmbAJAXMutableSpan optionselect_d("optionselect_d");

EmbAJAXSlider slider1("slider1", -range0, range0, 0); // change it into value input
EmbAJAXMutableSpan slider_d1("slider_d1");
char slider_d_buf1[BUFLEN];

EmbAJAXSlider slider2("slider2", -range0, range0, 0); // change it into value input
EmbAJAXMutableSpan slider_d2("slider_d2");
char slider_d_buf2[BUFLEN];

EmbAJAXStatic nextCell("</td><td>&nbsp;</td><td><b>");
EmbAJAXStatic nextRow("</b></td></tr><tr><td>");

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "Easy Control", "",
                 new EmbAJAXStatic("<h1>+++ Robot Console +++</h1>"),
                 new EmbAJAXStatic("<table cellpadding=\"10\"><tr><td>"),

                 new EmbAJAXStatic("Photodiode_1 analog value: "),
                 &nextCell,
                 &sensor_1,
                 &nextRow,

                 new EmbAJAXStatic("Photodiode_2 analog value: "),
                 &nextCell,
                 &sensor_2,
                 &nextRow,

                 &radio,
                 &nextCell,
                 &radio_d,
                 &nextRow,

                 new EmbAJAXStatic("motor_1 "),
                 &slider1,
                 &nextCell,
                 &slider_d1,
                 &nextRow,

                 new EmbAJAXStatic("motor_2 "),
                 &slider2,
                 &nextCell,
                 &slider_d2,
                 &nextRow,

                 new EmbAJAXStatic("Server status:"),
                 &nextCell,
                 new EmbAJAXConnectionIndicator(),
                 new EmbAJAXStatic("</b></td></tr></table>")
                )

void setup()
{
  pinMode(0, OUTPUT);
  pinMode(A_out1, OUTPUT);
  pinMode(A_out2, OUTPUT);
  pinMode(B_out1, OUTPUT);
  pinMode(B_out2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(16, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Connection wait
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    digitalWrite(16,!digitalRead(16));
  }
  // print some parameters pertaining to server IP etc.
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Tell the server to serve our EmbAJAX test page on root
  // installPage() abstracts over the (trivial but not uniform) WebServer-specific instructions to do so
  driver.installPage(&page, "/", updateUI);
  server.begin();

  updateUI(); // init displays
}

void updateUI() {
  // Update UI. Note that you could simply do this inside the loop. However,
  // placing it here makes the client UI more responsive (try it).
  radio_d.setValue(radio_opts[radio.selectedOption()]);
  optionselect_d.setValue(radio_opts[optionselect.selectedOption()]);
  slider_d1.setEnabled(radio.selectedOption() == 4);
  slider_d2.setEnabled(radio.selectedOption() == 4);
  slider_d1.setValue(itoa(slider1.intValue(), slider_d_buf1, 10));
  slider_d2.setValue(itoa(slider2.intValue(), slider_d_buf2, 10));
}

int step(int x) {
  if(x>0){
    return(1);
  } else {
    return(0);
  }
}
void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();
  digitalWrite(16,HIGH);
  digitalWrite(14,LOW);
  delay(3);
  sensorValue1 = analogRead(A0);
  //Serial.println(sensorValue1);
  delay(2);
  digitalWrite(14,HIGH);
  delay(3);
  sensorValue2 = analogRead(A0);
   Serial.println(sensorValue2);
  delay(2);
  sensor_1.setValue(itoa(sensorValue1, sensor_1_buf, 10));
  sensor_2.setValue(itoa(sensorValue2, sensor_2_buf, 10));
  
  if (radio.selectedOption() == 0) { //stop
    digitalWrite(A_out1,0);
    digitalWrite(A_out2,0);
    digitalWrite(B_out1,0);
    digitalWrite(B_out2,0);
  } else if (radio.selectedOption() == 1) { //spin
    analogWrite(A_out1, fix_value);
    digitalWrite(A_out2,0);
    analogWrite(B_out1, 0);
    digitalWrite(B_out2,fix_value);
  } else if (radio.selectedOption() == 2) { //ahead
    analogWrite(A_out1, 0);
    digitalWrite(A_out2,fix_value);
    analogWrite(B_out1, 0);
    digitalWrite(B_out2,fix_value);
  } else if (radio.selectedOption() == 3) { //astern
    analogWrite(A_out1, fix_value);
    digitalWrite(A_out2,0);
    analogWrite(B_out1, fix_value);
    digitalWrite(B_out2,0);
  } else { //user define mode
    if (slider1.intValue()>0) {
    digitalWrite(A_out1, 0);
    analogWrite(A_out2,abs(slider1.intValue()));
    } else {
    analogWrite(A_out1, abs(slider1.intValue()));
    digitalWrite(A_out2,0);
    }
    if (slider2.intValue()>0) {
    digitalWrite(B_out1, 0);
    analogWrite(B_out2,abs(slider2.intValue()));
    } else {
    analogWrite(B_out1, abs(slider2.intValue()));
    digitalWrite(B_out2,0);
    }
  }
}
