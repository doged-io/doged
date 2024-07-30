use bitcoinsuite_core::{
    AddressType, BytesMut, Hashed, Script, Sha256d, ShaRmd160,
};
use std::borrow::Cow;
use thiserror::Error;

use crate::chain::Chain;

const MAINNET_P2PKH_PREFIX: u8 = 30;
const TESTNET_P2PKH_PREFIX: u8 = 113;
const REGTEST_P2PKH_PREFIX: u8 = 111;
const MAINNET_P2SH_PREFIX: u8 = 22;
const TESTNET_P2SH_PREFIX: u8 = 196;

pub const DOGE_ADDRESS_CHECKSUM_LEN: usize = 4;

#[derive(Clone, Debug, Eq, PartialEq, Hash)]
pub struct DogeAddress<'a> {
    addr_type: AddressType,
    hash: ShaRmd160,
    addr: Cow<'a, str>,
    chain: &'a Chain,
}

#[derive(Error, Clone, Debug, Eq, PartialEq)]
pub enum DogeAddressError {
    #[error("Invalid base58")]
    InvalidBase58(bs58::decode::Error),
    #[error("Prefix missing from address")]
    MissingPrefix,
    #[error("Invalid payload length: {0}")]
    InvalidPayloadLength(usize),
    #[error("Invalid payload")]
    InvalidPayload,
    #[error("Invalid checksum")]
    InvalidChecksum,
    #[error("Invalid address type for prefix {0}")]
    InvalidAddressType(u8),
    #[error("Invalid chain")]
    InvalidChain,
    #[error("invalid chain or address type")]
    InvalidChainOrAddressType,
}

impl<'a> DogeAddress<'a> {
    pub fn from_hash(
        addr_type: AddressType,
        hash: ShaRmd160,
        chain: &'a Chain,
    ) -> Result<Self, DogeAddressError> {
        let prefix = match (chain, addr_type) {
            (Chain::Mainnet, AddressType::P2PKH) => MAINNET_P2PKH_PREFIX,
            (Chain::Testnet, AddressType::P2PKH) => TESTNET_P2SH_PREFIX,
            (Chain::Regtest, AddressType::P2PKH) => REGTEST_P2PKH_PREFIX,
            (Chain::Mainnet, AddressType::P2SH) => MAINNET_P2SH_PREFIX,
            (Chain::Testnet, AddressType::P2SH) => TESTNET_P2SH_PREFIX,
            _ => return Err(DogeAddressError::InvalidChainOrAddressType),
        };

        Ok(DogeAddress {
            addr: _to_doge_addr(prefix as u8, &hash.as_slice()).into(),
            addr_type,
            hash,
            chain,
        })
    }

    pub fn parse_cow(
        addr: Cow<'a, str>,
        curr_chain: &'a Chain,
    ) -> Result<Self, DogeAddressError> {
        let (hash, addr_type, chain) = _from_str(&addr)?;

        if chain != *curr_chain {
            return Err(DogeAddressError::InvalidChain);
        }
        Ok(DogeAddress {
            addr_type,
            hash: ShaRmd160::from_array(hash.into()),
            addr,
            chain: curr_chain,
        })
    }

    pub fn hash(&self) -> &ShaRmd160 {
        &self.hash
    }

    pub fn chain(&self) -> &Chain {
        &self.chain
    }

    pub fn as_str(&self) -> &str {
        &self.addr
    }

    pub fn into_string(self) -> String {
        self.addr.into_owned()
    }

    pub fn addr_type(&self) -> AddressType {
        self.addr_type
    }

    pub fn to_script(&self) -> Script {
        match self.addr_type {
            AddressType::P2PKH => Script::p2pkh(self.hash()),
            AddressType::P2SH => Script::p2sh(self.hash()),
        }
    }
}

fn _calculate_checksum(prefix: u8, payload: &[u8]) -> [u8; 4] {
    // The data that will be hashed for the checksum
    let mut checksum_preimage = BytesMut::new();
    checksum_preimage.put_slice(&[prefix]);
    checksum_preimage.put_slice(payload);
    let checksum_hash = Sha256d::digest(checksum_preimage.freeze());
    checksum_hash.as_slice()[..DOGE_ADDRESS_CHECKSUM_LEN]
        .try_into()
        .unwrap()
}

fn _verify_checksum(data: &[u8]) -> Result<bool, DogeAddressError> {
    let prefix = *data.first().ok_or(DogeAddressError::MissingPrefix)?;
    let checksum_start_idx = data
        .len()
        .checked_sub(DOGE_ADDRESS_CHECKSUM_LEN)
        .ok_or(DogeAddressError::InvalidPayloadLength(data.len()))?;

    let payload = data
        .get(1..checksum_start_idx)
        .ok_or(DogeAddressError::InvalidPayload)?;
    let actual_checksum = data
        .get(checksum_start_idx..data.len())
        .ok_or(DogeAddressError::InvalidChecksum)?;
    let expected_checksum = _calculate_checksum(prefix, payload);

    if expected_checksum != actual_checksum {
        return Err(DogeAddressError::InvalidChecksum);
    }
    Ok(true)
}

fn _to_doge_addr(prefix: u8, hash_bytes: &[u8]) -> String {
    let payload = [prefix]
        .iter()
        .chain(hash_bytes.iter())
        .cloned()
        .collect::<Vec<u8>>();
    let checksum = _calculate_checksum(prefix, hash_bytes);
    let mut data = BytesMut::new();
    data.put_slice(&payload);
    data.put_slice(&checksum);
    bs58::encode(data.as_slice()).into_string()
}

fn _from_str(
    addr_string: &str,
) -> Result<([u8; 20], AddressType, Chain), DogeAddressError> {
    let data = bs58::decode(&addr_string)
        .into_vec()
        .map_err(DogeAddressError::InvalidBase58)?;

    if !_verify_checksum(&data)? {
        return Err(DogeAddressError::InvalidChecksum);
    }

    let prefix = data[0];

    let address_type = match prefix {
        MAINNET_P2PKH_PREFIX | TESTNET_P2PKH_PREFIX | REGTEST_P2PKH_PREFIX => {
            AddressType::P2PKH
        }
        MAINNET_P2SH_PREFIX | TESTNET_P2SH_PREFIX => AddressType::P2SH,
        x => return Err(DogeAddressError::InvalidAddressType(x)),
    };

    let chain = match prefix {
        MAINNET_P2PKH_PREFIX | MAINNET_P2SH_PREFIX => Chain::Mainnet,
        TESTNET_P2PKH_PREFIX | TESTNET_P2SH_PREFIX => Chain::Testnet,
        REGTEST_P2PKH_PREFIX => Chain::Regtest,
        _ => return Err(DogeAddressError::InvalidChain),
    };

    let hash = &data[1..data.len() - DOGE_ADDRESS_CHECKSUM_LEN];
    let hash: [u8; 20] = match hash.try_into() {
        Ok(hash) => hash,
        Err(_) => {
            return Err(DogeAddressError::InvalidPayloadLength(hash.len()))
        }
    };
    Ok((hash, address_type, chain))
}
