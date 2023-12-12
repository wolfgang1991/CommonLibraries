#ifndef uCI2CHelpers_H_
#define uCI2CHelpers_H_

template <typename TI2CPort>
bool i2cReadBlock(TI2CPort& i2cPort, uint8_t device, uint8_t registerPointer, uint8_t length, uint8_t* buffer){
	//set register pointer
	i2cPort.beginTransmission(device);
	i2cPort.write(registerPointer);
	int error = i2cPort.endTransmission();
	//read data
	if(error==0){
		uint8_t received = i2cPort.requestFrom(device, length);
		uint8_t i=0;
		for(; i<length && i2cPort.available()>0; i++){
			buffer[i] = i2cPort.read();
		}
		return i==length && received==length;
	}
	return error==0;
}

template <typename TI2CPort>
bool i2cWriteRegister(TI2CPort& i2cPort, uint8_t device, uint8_t registerPointer, uint8_t content){
	i2cPort.beginTransmission(device);
	i2cPort.write(byte(registerPointer));
	i2cPort.write(byte(content));
	int error = i2cPort.endTransmission();
	return error==0;
}

#endif
