
// Struct to receive image data and allow capability to sort by imageIndex
struct TextureDataChunk {

	short totalChunks;
	short chunkIndex;
	int bufferLength;
	unsigned char* buffer;
	int lastPixelIndex;

};