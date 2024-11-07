import os
import sys
import re

def parse_file(log_file):
    idsmap = dict()
    tx = dict()
    rx = dict()
    rssi = dict()

    # regular expressions to match
    regex_id = re.compile(r".* (?P<node_id>\d+).firefly < b'Rime configured with address (?P<addr>\w\w:\w\w).*\n")
    regex_tx = re.compile(r".*TX (?P<addr>\w\w:\w\w).*\n")
    regex_rx = re.compile(r".*RX (?P<addr_from>\w\w:\w\w)->(?P<addr_to>\w\w:\w\w), RSSI = (?P<rssi>-?\d+)dBm.*\n")

    # open log and read line by line to match addresses to IDs
    with open(log_file, 'r') as f:
        for line in f:

            # match address strings to map them to IDs
            m = regex_id.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # add to the dictionary
                idsmap[d['addr']] = d['node_id']

    # open log and read line by line to count TX and RX
    with open(log_file, 'r') as f:
        for line in f:

            # match transmissions strings
            m = regex_tx.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # increase the count of transmissions from this address
                try:
                    tx_id = idsmap[d['addr']]
                    tx[tx_id] = tx.get(tx_id, 0) + 1
                except KeyError:
                    print(f"ID not found for address {d['addr']}")

                continue

            # match reception strings
            m = regex_rx.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # increase the count of receptions for the given link
                key_found = True
                try:
                    from_id = idsmap[d['addr_from']]
                except KeyError:
                    print(f"ID not found for address {d['addr_from']}")
                    key_found = False
                try:
                    to_id = idsmap[d['addr_to']]
                except KeyError:
                    print(f"ID not found for address {d['addr_to']}")
                    key_found = False
                
                if key_found:
                    rx_update = rx.get(from_id, dict())
                    rx_update[to_id] = rx_update.get(to_id, 0) + 1
                    rx[from_id] = rx_update

                    # collect RSSI measurements for the given link
                    rssi_update = rssi.get(from_id, dict())
                    rssi_update[to_id] = rssi_update.get(to_id, 0) + int(d['rssi'])
                    rssi[from_id] = rssi_update

                continue

    # diplay results
    for tx_id in tx:
        print(f"FROM {tx_id}\t\t{tx[tx_id]}")
        for rx_id in rx[tx_id]:
            print(f"\tTO {rx_id}\t{rx[tx_id][rx_id]}")
        print("\n")


if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("Error: Missing log file.")
        sys.exit(1)

    # get the log path and check that it exists
    log_file = sys.argv[1]
    if not os.path.isfile(log_file) or not os.path.exists(log_file):
        print("Error: Log file not found.")
        sys.exit(1)

    # parse file
    parse_file(log_file)
