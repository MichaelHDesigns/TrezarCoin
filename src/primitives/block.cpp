// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "crypto/neoscrypt.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "util.h"

uint256 CBlockHeader::GetHash() const
{
    if(idCached != ((uint64_t)nNonce << 32 | (uint64_t)nTime)) {
        /* BLAKE2s */

        /* 80 + 32 bytes, no padding */
        unsigned char input[112];
        /* Copy the block header */
        neoscrypt_copy(&input[0], &nVersion, 80);
        /* Copy the merkle root once again */
        neoscrypt_copy(&input[80], &hashMerkleRoot, 32);
        /* Hash the data;
         * key is higher and lower 10 bytes of merkle root
         * with nTime, nBits, nNonce in between */
        neoscrypt_blake2s(&input[0], 112, &input[58], 32, &hashCached, 32);

        idCached = ((uint64_t)nNonce << 32 | (uint64_t)nTime);
        nBlockHashCacheMisses++;
    } else {
        nBlockHashCacheHits++;
    }

    return(hashCached);
}

uint256 CBlockHeader::GetPoWHash() const
{
    unsigned int profile = 0x0;
    uint256 hash;

    profile |= nNeoScryptOptions;
    neoscrypt((unsigned char *) &nVersion, (unsigned char *) &hash, profile);

    return(hash);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i].ToString() << "\n";
    }
    return s.str();
}

int64_t GetBlockWeight(const CBlock& block)
{
    // This implements the weight = (stripped_size * 4) + witness_size formula,
    // using only serialization with and without witness data. As witness_size
    // is equal to total_size - stripped_size, this formula is identical to:
    // weight = (stripped_size * 3) + total_size.
    return ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
}
