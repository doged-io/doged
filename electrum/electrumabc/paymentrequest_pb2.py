# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: paymentrequest.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x14paymentrequest.proto\x12\x08payments\"+\n\x06Output\x12\x11\n\x06\x61mount\x18\x01 \x01(\x04:\x01\x30\x12\x0e\n\x06script\x18\x02 \x02(\x0c\"\xa3\x01\n\x0ePaymentDetails\x12\x15\n\x07network\x18\x01 \x01(\t:\x04main\x12!\n\x07outputs\x18\x02 \x03(\x0b\x32\x10.payments.Output\x12\x0c\n\x04time\x18\x03 \x02(\x04\x12\x0f\n\x07\x65xpires\x18\x04 \x01(\x04\x12\x0c\n\x04memo\x18\x05 \x01(\t\x12\x13\n\x0bpayment_url\x18\x06 \x01(\t\x12\x15\n\rmerchant_data\x18\x07 \x01(\x0c\"\x95\x01\n\x0ePaymentRequest\x12\"\n\x17payment_details_version\x18\x01 \x01(\r:\x01\x31\x12\x16\n\x08pki_type\x18\x02 \x01(\t:\x04none\x12\x10\n\x08pki_data\x18\x03 \x01(\x0c\x12\"\n\x1aserialized_payment_details\x18\x04 \x02(\x0c\x12\x11\n\tsignature\x18\x05 \x01(\x0c\"\'\n\x10X509Certificates\x12\x13\n\x0b\x63\x65rtificate\x18\x01 \x03(\x0c\"i\n\x07Payment\x12\x15\n\rmerchant_data\x18\x01 \x01(\x0c\x12\x14\n\x0ctransactions\x18\x02 \x03(\x0c\x12#\n\trefund_to\x18\x03 \x03(\x0b\x32\x10.payments.Output\x12\x0c\n\x04memo\x18\x04 \x01(\t\">\n\nPaymentACK\x12\"\n\x07payment\x18\x01 \x02(\x0b\x32\x11.payments.Payment\x12\x0c\n\x04memo\x18\x02 \x01(\tB(\n\x1eorg.bitcoin.protocols.paymentsB\x06Protos')

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'paymentrequest_pb2', _globals)
if not _descriptor._USE_C_DESCRIPTORS:
  _globals['DESCRIPTOR']._loaded_options = None
  _globals['DESCRIPTOR']._serialized_options = b'\n\036org.bitcoin.protocols.paymentsB\006Protos'
  _globals['_OUTPUT']._serialized_start=34
  _globals['_OUTPUT']._serialized_end=77
  _globals['_PAYMENTDETAILS']._serialized_start=80
  _globals['_PAYMENTDETAILS']._serialized_end=243
  _globals['_PAYMENTREQUEST']._serialized_start=246
  _globals['_PAYMENTREQUEST']._serialized_end=395
  _globals['_X509CERTIFICATES']._serialized_start=397
  _globals['_X509CERTIFICATES']._serialized_end=436
  _globals['_PAYMENT']._serialized_start=438
  _globals['_PAYMENT']._serialized_end=543
  _globals['_PAYMENTACK']._serialized_start=545
  _globals['_PAYMENTACK']._serialized_end=607
# @@protoc_insertion_point(module_scope)
