extern "C" {

/*
 * GetToken - function to obtain a token from the SecurityAgent
 *
 * Parameters
 *  maxLength   - holds the maximum uint8_t length of the buffer
 *  inLength    - holds the length of the current that needs to be tokenized.
 *  Id          - Buffer holds the data to tokenize on its way in, and returns in the same buffer the token.
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the token
 *  > 0 - success, char length of the returned token
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetToken(unsigned short maxLength, unsigned short inLength, unsigned char buffer[]);

}
