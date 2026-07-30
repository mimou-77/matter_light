#!/usr/bin/env python3
"""Generate the `fctry` NVS image (mfg.bin) from mfg_config.csv.

The output lands in out_mfg_bin_somef_<device-type>_<serial-num>/mfg.bin so that
each device build keeps its own image instead of overwriting a shared one.
"""
import argparse, csv, os, re, subprocess, sys

# csv key -> (nvs key, type, encoding). Encodings must match what
# ESP32FactoryDataProvider reads back, otherwise nvs returns TYPE_MISMATCH.
KEYS = [
    ('vendor-id',    'vendor-id',    'data', 'u32'),
    ('vendor-name',  'vendor-name',  'data', 'string'),
    ('product-id',   'product-id',   'data', 'u32'),
    ('product-name', 'product-name', 'data', 'string'),
    ('hw-ver',       'hw-ver',       'data', 'u32'),
    ('hw-ver-str',   'hw-ver-str',   'data', 'string'),
    ('serial-num',   'serial-num',   'data', 'string'),
    ('discriminator','discriminator','data', 'u32'),
    ('pin-code',     'pin-code',     'data', 'u32'),
    ('mfg-date',     'mfg-date',     'data', 'string'),
    ('unique-id',    'unique-id',    'data', 'hex2bin'),
]
NUMERIC = ('u8', 'i8', 'u16', 'i16', 'u32', 'i32', 'u64', 'i64')


def read_config(path):
    """Read the two-column `key,value` config, ignoring blanks and # comments."""
    if not os.path.isfile(path):
        sys.exit(f"❌ config file not found: {path}")
    cfg = {}
    with open(path, newline='') as f:
        for row in csv.reader(f):
            if not row or not row[0].strip() or row[0].lstrip().startswith('#'):
                continue
            key = row[0].strip()
            if key == 'key':  # header
                continue
            cfg[key] = row[1].strip() if len(row) > 1 else ''
    return cfg


def slug(value):
    return re.sub(r'[^a-z0-9]+', '_', value.lower()).strip('_')


parser = argparse.ArgumentParser()
parser.add_argument('--config', default='mfg_config.csv',
                    help='csv holding vendor-id, product-id, ... (default: mfg_config.csv)')
parser.add_argument('--vendor-id')
parser.add_argument('--product-id')
parser.add_argument('--vendor-name')
parser.add_argument('--product-name')
parser.add_argument('--device-type')
parser.add_argument('--serial-num')
parser.add_argument('--outdir', help='override the out_mfg_bin_somef_<type>_<serial> folder')
parser.add_argument('--size', default='0x6000', help='fctry partition size (default: 0x6000)')
args = parser.parse_args()

cfg = read_config(args.config)
# command line wins over the csv
for name in ('vendor_id', 'product_id', 'vendor_name', 'product_name', 'device_type', 'serial_num'):
    value = getattr(args, name)
    if value:
        cfg[name.replace('_', '-')] = value

device_type = cfg.get('device-type', '')
serial_num = cfg.get('serial-num', '')
if not device_type or not serial_num:
    sys.exit(f"❌ 'device-type' and 'serial-num' are required in {args.config}")

outdir = args.outdir or f"out_mfg_bin_somef_{slug(device_type)}_{slug(serial_num)}"
os.makedirs(outdir, exist_ok=True)

nvs_csv = os.path.join(outdir, 'temp_nvs.csv')
written = []
with open(nvs_csv, 'w', newline='') as f_out:
    writer = csv.writer(f_out)
    writer.writerow(['key', 'type', 'encoding', 'value'])
    writer.writerow(['chip-factory', 'namespace', '', ''])
    for cfg_key, nvs_key, nvs_type, encoding in KEYS:
        value = cfg.get(cfg_key, '')
        if not value:
            continue
        if encoding in NUMERIC:
            try:
                # nvs_partition_gen parses with int(value), so 0x.. must be expanded here
                value = str(int(value, 0))
            except ValueError:
                sys.exit(f"❌ '{cfg_key}' is not a number: {cfg[cfg_key]}")
        writer.writerow([nvs_key, nvs_type, encoding, value])
        written.append(nvs_key)

idf_path = os.environ.get('IDF_PATH', '/home/mariem/esp-idf')
gen_script = os.path.join(idf_path, 'components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py')
out_bin = os.path.join(outdir, 'mfg.bin')

subprocess.run(['python3', gen_script, 'generate', nvs_csv, out_bin, args.size], check=True)
os.remove(nvs_csv)
print(f"✅ {out_bin} ({len(written)} keys: {', '.join(written)})")
