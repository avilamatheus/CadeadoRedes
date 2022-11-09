#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool moniStarted = false;
bool adver = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Dispositivo conectado!");      
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Dispositivo desconectado!");
      adver = false;
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  pinMode(5, INPUT);

  //setup motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(5); //Accelerometer sensitivity to motion
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

    // Create the BLE Device
  BLEDevice::init("ESP32-Cadeado");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
}

void loop() {
  if(isLocked()) {
    if(adver == false) {
      pServer->startAdvertising();
      adver = true;
    }
    if(!moniStarted && deviceConnected){
      moniStarted = true;
      Serial.println("Monit. iniciado");
    }
    if(mpu.getMotionInterruptStatus()) {
        pCharacteristic->setValue("Movim. detectada!");
        Serial.println("Movim. detectada!");
        pCharacteristic->notify();
        delay(1000);
        pCharacteristic->setValue("");
        pCharacteristic->notify();
    }
  } else if (moniStarted && !isLocked()){
    Serial.println("Monit. finalizado");    
    moniStarted = false;
  } else if (!isLocked() && adver == true) {
    pServer->getAdvertising()->stop();
    adver = false;
  } else {
    moniStarted = false;
  }
}

bool isLocked() {
    int chave = 0;
    chave = digitalRead(5);
    if(chave == 1){
      return true;
    }
    else {
      return false;
    }
}
