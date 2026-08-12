#include "MJPEGRTPPayloadCreator.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

constexpr uint8_t markerPrefix = 0xFF;
constexpr uint8_t soiMarker = 0xD8;
constexpr uint8_t eoiMarker = 0xD9;
constexpr uint8_t sof0Marker = 0xC0;
constexpr uint8_t dhtMarker = 0xC4;
constexpr uint8_t dqtMarker = 0xDB;
constexpr uint8_t driMarker = 0xDD;
constexpr uint8_t sosMarker = 0xDA;

constexpr uint8_t defaultQValue = 255;
constexpr uint16_t quantizationDataLength = 128;

static const uint8_t lumDcCodeLens[] = {
	0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t lumDcSymbols[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const uint8_t lumAcCodeLens[] = {
	0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

static const uint8_t lumAcSymbols[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,
};

static const uint8_t chmDcCodeLens[] = {
	0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static const uint8_t chmDcSymbols[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const uint8_t chmAcCodeLens[] = {
	0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static const uint8_t chmAcSymbols[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,
};

struct QuantizationTable {
	bool valid;
	uint8_t values[64];

	QuantizationTable() : valid(false), values{} {}
};

struct ParsedJpeg {
	uint16_t width;
	uint16_t height;
	uint16_t restartInterval;
	uint8_t type;
	uint8_t lqt[64];
	uint8_t cqt[64];
	uint32_t scanDataOffset;
	uint32_t scanDataLength;

	ParsedJpeg()
		: width(0), height(0), restartInterval(0), type(0), lqt{}, cqt{}, scanDataOffset(0), scanDataLength(0) {}
};

uint16_t readBigEndian16(const uint8_t* data) {
	return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]));
}

void invalidatePayload(MJPEGRTPPayload& payload) {
	payload.totalLength = 0;
}

void writeFragmentOffset(uint8_t* target, uint32_t fragmentOffset) {
	target[0] = static_cast<uint8_t>((fragmentOffset >> 16) & 0xFF);
	target[1] = static_cast<uint8_t>((fragmentOffset >> 8) & 0xFF);
	target[2] = static_cast<uint8_t>(fragmentOffset & 0xFF);
}

bool isDefaultHuffmanTable(uint8_t tableClass, uint8_t tableId, const uint8_t* codeLens, uint32_t symbolCount, const uint8_t* symbols) {
	if (tableClass == 0 && tableId == 0) {
		return std::memcmp(codeLens, lumDcCodeLens, sizeof(lumDcCodeLens)) == 0
			&& symbolCount == sizeof(lumDcSymbols)
			&& std::memcmp(symbols, lumDcSymbols, sizeof(lumDcSymbols)) == 0;
	}
	if (tableClass == 1 && tableId == 0) {
		return std::memcmp(codeLens, lumAcCodeLens, sizeof(lumAcCodeLens)) == 0
			&& symbolCount == sizeof(lumAcSymbols)
			&& std::memcmp(symbols, lumAcSymbols, sizeof(lumAcSymbols)) == 0;
	}
	if (tableClass == 0 && tableId == 1) {
		return std::memcmp(codeLens, chmDcCodeLens, sizeof(chmDcCodeLens)) == 0
			&& symbolCount == sizeof(chmDcSymbols)
			&& std::memcmp(symbols, chmDcSymbols, sizeof(chmDcSymbols)) == 0;
	}
	if (tableClass == 1 && tableId == 1) {
		return std::memcmp(codeLens, chmAcCodeLens, sizeof(chmAcCodeLens)) == 0
			&& symbolCount == sizeof(chmAcSymbols)
			&& std::memcmp(symbols, chmAcSymbols, sizeof(chmAcSymbols)) == 0;
	}
	return false;
}

bool parseDqtSegment(const uint8_t* segment, uint16_t segmentLength, QuantizationTable* tables) {
	uint32_t offset = 0;
	while (offset < segmentLength) {
		const uint8_t tableInfo = segment[offset++];
		const uint8_t precision = static_cast<uint8_t>(tableInfo >> 4);
		const uint8_t tableId = static_cast<uint8_t>(tableInfo & 0x0F);
		if (precision != 0 || tableId >= 4 || offset + 64 > segmentLength) {
			return false;
		}
		std::memcpy(tables[tableId].values, &(segment[offset]), 64);
		tables[tableId].valid = true;
		offset += 64;
	}
	return true;
}

bool parseDhtSegment(const uint8_t* segment, uint16_t segmentLength) {
	uint32_t offset = 0;
	while (offset < segmentLength) {
		if (offset + 17 > segmentLength) {
			return false;
		}
		const uint8_t tableInfo = segment[offset++];
		const uint8_t tableClass = static_cast<uint8_t>(tableInfo >> 4);
		const uint8_t tableId = static_cast<uint8_t>(tableInfo & 0x0F);
		const uint8_t* codeLens = &(segment[offset]);
		offset += 16;
		uint32_t symbolCount = 0;
		for (uint8_t i = 0; i < 16; ++i) {
			symbolCount += codeLens[i];
		}
		if (offset + symbolCount > segmentLength) {
			return false;
		}
		if (!isDefaultHuffmanTable(tableClass, tableId, codeLens, symbolCount, &(segment[offset]))) {
			return false;
		}
		offset += symbolCount;
	}
	return true;
}

bool parseSof0Segment(const uint8_t* segment, uint16_t segmentLength, const QuantizationTable* tables, ParsedJpeg& parsed) {
	if (segmentLength < 15 || segment[0] != 8) {
		return false;
	}

	const uint16_t height = readBigEndian16(&(segment[1]));
	const uint16_t width = readBigEndian16(&(segment[3]));
	const uint8_t componentCount = segment[5];
	if (componentCount != 3 || segmentLength != static_cast<uint16_t>(6 + componentCount * 3)) {
		return false;
	}

	uint8_t lumaTableId = 0xFF;
	uint8_t chromaTableId = 0xFF;
	uint8_t type = 0xFF;
	uint8_t chromaComponents = 0;

	for (uint8_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
		const uint32_t componentOffset = static_cast<uint32_t>(6 + componentIndex * 3);
		const uint8_t sampling = segment[componentOffset + 1];
		const uint8_t tableId = segment[componentOffset + 2];
		if (sampling == 0x21) {
			if (type != 0xFF) {
				return false;
			}
			type = 0;
			lumaTableId = tableId;
		} else if (sampling == 0x22) {
			if (type != 0xFF) {
				return false;
			}
			type = 1;
			lumaTableId = tableId;
		} else if (sampling == 0x11) {
			if (chromaTableId == 0xFF) {
				chromaTableId = tableId;
			} else if (chromaTableId != tableId) {
				return false;
			}
			++chromaComponents;
		} else {
			return false;
		}
	}

	if (type == 0xFF || chromaComponents != 2 || lumaTableId >= 4 || chromaTableId >= 4) {
		return false;
	}
	if (!tables[lumaTableId].valid || !tables[chromaTableId].valid) {
		return false;
	}

	parsed.width = width;
	parsed.height = height;
	parsed.type = type;
	std::memcpy(parsed.lqt, tables[lumaTableId].values, sizeof(parsed.lqt));
	std::memcpy(parsed.cqt, tables[chromaTableId].values, sizeof(parsed.cqt));
	return true;
}

bool parseSosSegment(const uint8_t* segment, uint16_t segmentLength) {
	if (segmentLength != 10 || segment[0] != 3) {
		return false;
	}

	uint8_t lumaSelectors = 0;
	uint8_t chromaSelectors = 0;
	uint8_t chromaCount = 0;
	for (uint8_t componentIndex = 0; componentIndex < 3; ++componentIndex) {
		const uint32_t componentOffset = static_cast<uint32_t>(1 + componentIndex * 2);
		const uint8_t tableSelectors = segment[componentOffset + 1];
		if (tableSelectors == 0x00) {
			++lumaSelectors;
		} else if (tableSelectors == 0x11) {
			++chromaSelectors;
			++chromaCount;
		} else {
			return false;
		}
	}
	return lumaSelectors == 1 && chromaSelectors == 2 && chromaCount == 2
		&& segment[7] == 0 && segment[8] == 63 && segment[9] == 0;
}

bool findScanDataEnd(const uint8_t* jpegBuf, uint32_t jpegSize, uint32_t scanDataOffset, uint32_t& scanDataLength) {
	for (uint32_t offset = scanDataOffset; offset + 1 < jpegSize; ++offset) {
		if (jpegBuf[offset] != markerPrefix) {
			continue;
		}
		const uint8_t next = jpegBuf[offset + 1];
		if (next == 0x00) {
			++offset;
			continue;
		}
		if (next >= 0xD0 && next <= 0xD7) {
			++offset;
			continue;
		}
		if (next == eoiMarker) {
			scanDataLength = offset - scanDataOffset;
			return true;
		}
		return false;
	}
	return false;
}

bool parseJpeg(const uint8_t* jpegBuf, uint32_t jpegSize, ParsedJpeg& parsed) {
	if (jpegBuf == nullptr || jpegSize < 4 || jpegBuf[0] != markerPrefix || jpegBuf[1] != soiMarker) {
		return false;
	}

	QuantizationTable tables[4];
	bool foundSof = false;

	for (uint32_t offset = 2; offset + 1 < jpegSize;) {
		if (jpegBuf[offset] != markerPrefix) {
			return false;
		}

		while (offset < jpegSize && jpegBuf[offset] == markerPrefix) {
			++offset;
		}
		if (offset >= jpegSize) {
			return false;
		}

		const uint8_t marker = jpegBuf[offset++];
		if (marker == eoiMarker) {
			return false;
		}
		if (marker == soiMarker) {
			continue;
		}
		if (marker >= 0xD0 && marker <= 0xD7) {
			continue;
		}
		if (offset + 1 >= jpegSize) {
			return false;
		}

		const uint16_t segmentLength = readBigEndian16(&(jpegBuf[offset]));
		if (segmentLength < 2 || offset + segmentLength > jpegSize) {
			return false;
		}
		const uint8_t* segment = &(jpegBuf[offset + 2]);
		const uint16_t segmentDataLength = static_cast<uint16_t>(segmentLength - 2);

		if (marker == dqtMarker) {
			if (!parseDqtSegment(segment, segmentDataLength, tables)) {
				return false;
			}
		} else if (marker == dhtMarker) {
			if (!parseDhtSegment(segment, segmentDataLength)) {
				return false;
			}
		} else if (marker == driMarker) {
			if (segmentDataLength != 2) {
				return false;
			}
			parsed.restartInterval = readBigEndian16(segment);
		} else if (marker == sof0Marker) {
			if (!parseSof0Segment(segment, segmentDataLength, tables, parsed)) {
				return false;
			}
			foundSof = true;
		} else if (marker == sosMarker) {
			if (!foundSof || !parseSosSegment(segment, segmentDataLength)) {
				return false;
			}
			parsed.scanDataOffset = offset + segmentLength;
			if (!findScanDataEnd(jpegBuf, jpegSize, parsed.scanDataOffset, parsed.scanDataLength) || parsed.scanDataLength == 0) {
				return false;
			}
			if (parsed.width == 0 || parsed.height == 0) {
				return false;
			}
			return true;
		} else if (marker == 0xC1 || marker == 0xC2 || marker == 0xC3 || marker == 0xC5 || marker == 0xC6 || marker == 0xC7
			|| marker == 0xC9 || marker == 0xCA || marker == 0xCB || marker == 0xCD || marker == 0xCE || marker == 0xCF) {
			return false;
		}

		offset += segmentLength;
	}

	return false;
}

} // namespace

void createMJPEGRTPPayload(MJPEGRTPPayload& payload, uint8_t* jpegBuf, uint32_t jpegSize) {
	ParsedJpeg parsed;
	if (!parseJpeg(jpegBuf, jpegSize, parsed)) {
		invalidatePayload(payload);
		return;
	}

	const uint32_t widthBlocks = (static_cast<uint32_t>(parsed.width) + 7U) / 8U;
	const uint32_t heightBlocks = (static_cast<uint32_t>(parsed.height) + 7U) / 8U;
	if (widthBlocks == 0 || widthBlocks > 255 || heightBlocks == 0 || heightBlocks > 255 || payload.packetPayloadSize == 0) {
		invalidatePayload(payload);
		return;
	}

	uint8_t type = parsed.type;
	const uint32_t baseHeaderLength = 8;
	uint32_t restartHeaderLength = 0;
	if (parsed.restartInterval != 0) {
		type = static_cast<uint8_t>(type + 64);
		restartHeaderLength = 4;
	}
	const uint32_t quantizationHeaderLength = 4 + quantizationDataLength;
	const uint32_t firstPacketHeaderLength = baseHeaderLength + restartHeaderLength + quantizationHeaderLength;
	const uint32_t followingPacketHeaderLength = baseHeaderLength + restartHeaderLength;
	if (payload.packetPayloadSize <= firstPacketHeaderLength || payload.packetPayloadSize <= followingPacketHeaderLength) {
		invalidatePayload(payload);
		return;
	}

	const uint32_t firstPacketScanCapacity = payload.packetPayloadSize - firstPacketHeaderLength;
	const uint32_t followingPacketScanCapacity = payload.packetPayloadSize - followingPacketHeaderLength;
	uint32_t remainingScanBytes = parsed.scanDataLength;
	uint32_t packetCount = 1;
	if (remainingScanBytes > firstPacketScanCapacity) {
		remainingScanBytes -= firstPacketScanCapacity;
		packetCount += (remainingScanBytes + followingPacketScanCapacity - 1) / followingPacketScanCapacity;
	}

	const uint32_t totalPayloadLength = parsed.scanDataLength
		+ packetCount * (baseHeaderLength + restartHeaderLength)
		+ quantizationHeaderLength;
	const uint32_t requiredLength = RTPSender::headerSize + totalPayloadLength;

	if (payload.buf == nullptr || payload.bufferLength < requiredLength) {
		uint8_t* resizedBuffer = static_cast<uint8_t*>(std::realloc(payload.buf, requiredLength));
		if (resizedBuffer == nullptr) {
			invalidatePayload(payload);
			return;
		}
		payload.buf = resizedBuffer;
		payload.bufferLength = requiredLength;
	}

	uint32_t offset = RTPSender::headerSize;
	uint32_t scanOffset = 0;
	for (uint32_t packetIndex = 0; packetIndex < packetCount; ++packetIndex) {
		const bool isFirstPacket = packetIndex == 0;
		const uint32_t packetHeaderLength = isFirstPacket ? firstPacketHeaderLength : followingPacketHeaderLength;
		const uint32_t packetScanCapacity = isFirstPacket ? firstPacketScanCapacity : followingPacketScanCapacity;
		const uint32_t packetScanLength = std::min(parsed.scanDataLength - scanOffset, packetScanCapacity);

		payload.buf[offset++] = 0;
		writeFragmentOffset(&(payload.buf[offset]), scanOffset);
		offset += 3;
		payload.buf[offset++] = type;
		payload.buf[offset++] = defaultQValue;
		payload.buf[offset++] = static_cast<uint8_t>(widthBlocks);
		payload.buf[offset++] = static_cast<uint8_t>(heightBlocks);

		if (restartHeaderLength != 0) {
			payload.buf[offset++] = static_cast<uint8_t>(parsed.restartInterval >> 8);
			payload.buf[offset++] = static_cast<uint8_t>(parsed.restartInterval & 0xFF);
			payload.buf[offset++] = 0xC0;
			payload.buf[offset++] = 0x00;
		}

		if (isFirstPacket) {
			payload.buf[offset++] = 0;
			payload.buf[offset++] = 0;
			payload.buf[offset++] = static_cast<uint8_t>(quantizationDataLength >> 8);
			payload.buf[offset++] = static_cast<uint8_t>(quantizationDataLength & 0xFF);
			std::memcpy(&(payload.buf[offset]), parsed.lqt, sizeof(parsed.lqt));
			offset += sizeof(parsed.lqt);
			std::memcpy(&(payload.buf[offset]), parsed.cqt, sizeof(parsed.cqt));
			offset += sizeof(parsed.cqt);
		}

		std::memcpy(&(payload.buf[offset]), &(jpegBuf[parsed.scanDataOffset + scanOffset]), packetScanLength);
		offset += packetScanLength;
		scanOffset += packetScanLength;
		(void)packetHeaderLength;
	}

	payload.totalLength = requiredLength;
}