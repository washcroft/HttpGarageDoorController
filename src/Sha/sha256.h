#ifndef Sha256_h
#define Sha256_h

#include "sha256_config.h"

#if defined(SHA256_LINUX)
class Sha256Class
#else
class Sha256Class : public Print
#endif
{
  public:
	union _buffer {
		uint8_t b[BLOCK_LENGTH];
		uint32_t w[BLOCK_LENGTH/4];
	};
	union _state {
		uint8_t b[HASH_LENGTH];
		uint32_t w[HASH_LENGTH/4];
	};
    /** Initialized the SHA256 process
	 *  Set the counter and buffers
	 */
    void init(void);
	/** initializes the HMAC process for SHA-256
	 *
	 * @param secret The key to be used
	 * @param secretLength The length of the key
	 */
    void initHmac(const uint8_t* secret, int secretLength);
	
	/** Pads the last block and finalizes the hash. 
	 * 
	 * @return returns the hash
	 */
	uint8_t* result(void);
	
	/** Pads the last block and finalizes the hash. 
	 * 
	 * @return returns the hash
	 */
    uint8_t* resultHmac(void);
    #if  defined(SHA256_LINUX)
	/**
	 * Adds data to the buffer.
	 * Also increases the byteCount variable
	 *
	 */
	virtual size_t write(uint8_t);
	
	/**
	 * Adds the str to the buffer
	 * Calles function in order to add the data into the buffer.
	 * @param str The string to be added
	 * @note Print class does not exist in linux, so we define it here using
	 * @code #if defined(SHA1_LINUX) @endcode
	 *
	 */
	size_t write_L(const char *str);
	
	/**
	 * Adds the string to the buffer
	 * Calles function in order to add the data into the buffer.
	 * @param *buffer The string to be added
	 * @param *size The size of the string
	 * @note Print class does not exist in linux, so we define it here using
	 * @code #if defined(SHA1_LINUX) @endcode
	 *
	 */
	size_t write_L(const uint8_t *buffer, size_t size);
	
	/**
	 * Adds the str to the buffer
	 * Calles functions in order to add the data into the buffer.
	 * @param str The string to be added
	 * @note Print class does not exist in linux, so we define it here using
	 * @code #if defined(SHA1_LINUX) @endcode
	 *
	 */
	size_t print(const char* str);
	
	/**
	 * used in linux in order to retrieve the time in milliseconds.
	 *
	 * @return returns the milliseconds in a double format.
	 */
	double millis();
    #else
	/**
	 * Adds data to the buffer.
	 * Also increases the byteCount variable
	 *
	 */
	virtual size_t write(uint8_t);
	using Print::write;
    #endif
  private:
    /** Implement SHA-1 padding (fips180-2 ยง5.1.1).
	 *  Pad with 0x80 followed by 0x00 until the end of the block
	 *
	 */
    void pad();
	
	/** adds the data to the buffer
	 * 
	 * @param data 
	 *
	 */
    void addUncounted(uint8_t data);
	
	/** Hash a single block of data
	 *
	 */
    void hashBlock();
	
	/**
     * ror32 - rotate a 32-bit value left
     * @param number value to rotate
     * @param bits bits to roll
     */
    uint32_t ror32(uint32_t number, uint8_t bits);
    _buffer buffer;/**< hold the buffer for the hashing process */
    uint8_t bufferOffset;/**< indicates the position on the buffer */
    _state state;/**< identical structure with buffer */
    uint32_t byteCount;/**< Byte counter in order to initialize the hash process for a block */
    uint8_t keyBuffer[BLOCK_LENGTH];/**< Hold the key for the HMAC process*/
    uint8_t innerHash[HASH_LENGTH];/**< holds the inner hash for the HMAC process */
    #if defined(SHA256_LINUX)
		timeval tv;/**< hold the time value on linux machines (ex Raspberry Pi) */
	#endif
};
extern Sha256Class Sha256;

#endif
