import copy, os, statistics
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

LOG_DIR = "/home/cs310/oculus_server_log"
LOG_DIR = "logs"
RESULTS_DIR = "results"

if not os.path.exists(RESULTS_DIR):
    os.makedirs(RESULTS_DIR)

# locations = ["log_rain1", "log_rain3", "log_rain11", "log_rain12", "log_rain13", "log_rain14", "log_rain15","log_rain16"]
locations = ["ATL", "CHI", "DEN", "DFW", "LAX", "NYC", "SJC", "WAS"]
servers = {
    1:"SJC", 
    3:"DEN", 
    11:"LAX", 
    12:"WAS", 
    13:"CHI", 
    14:"DFW",
    15:"ATL", 
    16:"NYC" 
}

# Exmaples:
# key=STATISTICS type=handle server=11 time=2377729176 ingest=15 hash=758758780
# key=STATISTICS type=receive server=1 time=-1917178248 hash=1574997132
# key=STATISTICS type=ping value=85000 player=Joe
# key=STATISTICS type=stats start=1651009282802000 end=1651009282925000 hash=1539884390 player=Joe

STATISTICS_TEMPLATE = {
    'ingest': "UNKNOWN",
    'player': 'UNKNOWN',
    'ref_time': 0
}
for rain_id, server in servers.items():
    STATISTICS_TEMPLATE[server] = {
        'start': None,
        'receive': None,
        'handle': None,
        'end': None,
    }

PLAYER_TEMPLATE = {
    'pings': [],
    'rtts': []
}

player_map = {}
hash_map = {}
for location in locations:
    filename = f"{LOG_DIR}/{location}.log"
    file = open(filename, "r")
    lines = file.readlines()
    stat_lines = [line for line in filter(lambda x: "STATISTICS" in x, lines[:-10])]
    print(f"Reading from file \"{filename}\" [ lines={len(lines)} stats={len(stat_lines)} ] ", flush=True)
    for line in stat_lines:
        line = line.replace("\n", "")
        pairs = line.split(" ")
        obj = {}
        for pair in pairs:
            kv = pair.split("=")
            if len(kv) != 2:
                print(line)
                print(kv)
                break
            key, value = pair.split("=")
            obj[key] = value
        if obj['type'] == 'ping':
            p = obj['player']
            if p not in player_map.keys():
                player_map[p] = copy.deepcopy(PLAYER_TEMPLATE)
            player_map[p]['pings'].append(int(obj['value']))
        elif obj['type'] == 'handle':
            h = obj['hash']
            if h not in hash_map.keys():
                hash_map[h] = copy.deepcopy(STATISTICS_TEMPLATE)
            hash_map[h][location]['handle'] = int(obj['time'])
            hash_map[h]['ingest'] = servers[int(obj['ingest'])]
        elif obj['type'] == 'receive':
            h = obj['hash']
            if h not in hash_map.keys():
                hash_map[h] = copy.deepcopy(STATISTICS_TEMPLATE)
            hash_map[h][location]['receive'] = int(obj['time'])
        elif obj['type'] == 'stats':
            h = obj['hash']
            if h not in hash_map.keys():
                hash_map[h] = copy.deepcopy(STATISTICS_TEMPLATE)
            hash_map[h][location]['start'] = int(obj['start'])
            hash_map[h][location]['end'] = int(obj['end'])
            hash_map[h]['player'] = obj['player']
            p = obj['player']
            if p not in player_map.keys():
                player_map[p] = copy.deepcopy(PLAYER_TEMPLATE)
            player_map[p]['rtts'].append(int(obj['end']) - int(obj['start']))
        else:
            print(f"Unknown type={obj['type']}")

file = open(f"{RESULTS_DIR}/player_stats.txt", "w")

range_max = 60
bins = np.linspace(0, range_max, range_max * 4)
for player, stats in player_map.items():
    pings = [p / 1000 for p in stats['pings']]
    ping_avg = statistics.mean(pings)
    ping_stdev = statistics.stdev(pings)
    plt.hist(pings, bins, alpha=0.8, label=player)
    print(f"player={player} ping_avg={ping_avg:.3f}ms ping_stdev={ping_stdev:.3f}ms")
    file.write(f"player={player} ping_avg={ping_avg:.3f}ms ping_stdev={ping_stdev:.3f}ms\n")
plt.ylabel('Num Pings')
plt.xlabel('Ping Time (ms)')
plt.title('Client-Server Player Pings')
plt.legend()
plt.savefig(f"{RESULTS_DIR}/player_pings.png")
plt.clf()

range_max = 120
bins = np.linspace(0, range_max, range_max * 4)
for player, stats in player_map.items():
    rtts = [r / 1000 for r in stats['rtts']]
    rtt_avg = statistics.mean(rtts)
    rtt_stdev = statistics.stdev(rtts)
    plt.hist(rtts, bins, alpha=0.7, label=player)
    print(f"player={player} rtt_avg={rtt_avg:.3f}ms rtt_stdev={rtt_stdev:.3f}ms")
    file.write(f"player={player} rtt_avg={rtt_avg:.3f}ms rtt_stdev={rtt_stdev:.3f}ms\n")
plt.ylabel('Num Requests')
plt.xlabel('Round Trip Delay (ms)')
plt.title('Player Round Trip Delays')
plt.legend()
plt.savefig(f"{RESULTS_DIR}/player_rtts.png")
plt.clf()

file.close()

data = {}
for hash, stats in hash_map.items():
    for server, value in stats.items():
        if server in locations:
            if value['start'] and value['end'] and value['handle'] and value['receive']:
                data[hash] = stats
                break

for hash, stats in data.items():
    ingest = stats['ingest']
    ref_time = stats[ingest]['receive']
    stats['ref_time'] = ref_time
    for server, value in stats.items():
        if server in locations:
            value['start'] = value['start'] - ref_time if value['start'] else value['start']
            value['end'] = value['end'] - ref_time if value['end'] else value['end']
            value['handle'] = value['handle'] - ref_time if value['handle'] else value['handle']
            value['receive'] = value['receive'] - ref_time if value['receive'] else value['receive']

headers = ['player', 'hash', 'ingest', 'start', 'end']
for loc in locations:
    headers.append(f"{loc}_receive")
    headers.append(f"{loc}_handle")
headers.append('ref_time')
df_data = {}
for header in headers:
    df_data[header] = []

for hash, stats in data.items():
    df_data['hash'].append(hash)
    df_data['player'].append(stats['player'])
    df_data['ref_time'].append(stats['ref_time'])
    ingest = stats['ingest']
    df_data['ingest'].append(ingest)
    df_data['start'].append(stats[ingest]['start'])
    df_data['end'].append(stats[ingest]['end'])
    for loc in locations:
        df_data[f"{loc}_receive"].append(stats[loc]['receive'])
        df_data[f"{loc}_handle"].append(stats[loc]['handle'])

df = pd.DataFrame.from_dict(df_data)
df.to_csv(f"{RESULTS_DIR}/log_stats.csv")

for player in player_map.keys():
    df_loc = df.loc[df['player'] == player]
    data = []
    DATUM_TEMPLATE = {
        'ref_time': 0,
        'client_send': 0,
        'server_recv': 0,
        'server_send': 0,
        'client_recv': 0
    }
    for index, row in df_loc.iterrows():
        ingest = row['ingest']
        datum = copy.deepcopy(DATUM_TEMPLATE)
        datum['ref_time'] = row['ref_time']
        datum['client_send'] = row['start']
        datum['server_recv'] = row[f"{ingest}_receive"]
        datum['server_send'] = row[f"{ingest}_handle"]
        datum['client_recv'] = row['end']
        data.append(datum)

    data = sorted(data, key=lambda datum: datum['ref_time'])

    xs = [datum['ref_time'] / 1000000 for datum in data]
    start_x = xs[0]
    xs = [x - start_x for x in xs]
    plt.plot(xs, [datum['client_send'] / 1000 for datum in data], label='client_send')
    plt.plot(xs, [datum['server_recv'] / 1000 for datum in data], label='server_recv')
    plt.plot(xs, [datum['server_send'] / 1000 for datum in data], label='server_send')
    plt.plot(xs, [datum['client_recv'] / 1000 for datum in data], label='client_recv')
    plt.ylabel("Event Time Relative to server_recv (ms)")
    plt.xlabel("Time in Game (sec)")
    plt.title(f"Event Lifecycle Timestamps for {player}")
    plt.legend()
    plt.savefig(f"{RESULTS_DIR}/player_events_{player}.png")
    plt.clf()

range_max = 50
bins = np.linspace(0, range_max, range_max * 4)
for src in locations:
    df_loc = df.loc[df['ingest'] == src]
    if len(df_loc.index) == 0:
        continue  # nobody used this as ingest server
    for dst in locations:
        if src == dst:
            continue
        data = [x / 1000 for x in list(df_loc[f"{dst}_receive"])]
        plt.hist(data, bins, alpha=0.7, density=False, label=dst)  # density=False would make counts
    
    plt.xlim([0, range_max])
    plt.ylabel('Num Client Requests')
    plt.xlabel('Time Delta from Ingest (ms)')
    plt.title(f'Receiving from Ingest Server {src}')
    plt.legend()
    plt.savefig(f"{RESULTS_DIR}/{src}_receive.png")
    plt.clf()

sync_delay = 65
spread = 5
dense_bins = np.linspace(sync_delay - spread, sync_delay + spread, spread * 8)
spread_bins = np.linspace(sync_delay - spread, sync_delay + spread, spread * 2)
for src in locations:
    df_loc = df.loc[df['ingest'] == src]
    if len(df_loc.index) == 0:
        continue  # nobody used this as ingest server

    all_data = []
    all_labels = []
    for dst in locations:
        data = [x / 1000 for x in list(df_loc[f"{dst}_handle"])]
        all_data.append(data)
        all_labels.append(dst)
        plt.hist(data, dense_bins, alpha=0.7, label=dst)  # density=False would make counts

    plt.xlim([sync_delay - spread, sync_delay + spread])
    plt.ylabel('Num Client Requests')
    plt.xlabel('Time Delta from Ingest (ms)')
    plt.title(f'Handling from Ingest Server {src}')
    plt.legend()
    plt.savefig(f"{RESULTS_DIR}/{src}_handle_dense.png")
    plt.clf()
        
    plt.hist(all_data, spread_bins, alpha=1, label=all_labels)
    plt.xlim([sync_delay - spread, sync_delay + spread])
    plt.ylabel('Num Client Requests')
    plt.xlabel('Time Delta from Ingest (ms)')
    plt.title(f'Handling from Ingest Server {src}')
    plt.legend()
    plt.savefig(f"{RESULTS_DIR}/{src}_handle_spread.png")
    plt.clf()



    """
    for index, row in df_loc.iterrows():
        ingest = row['ingest']
        ref_time = row['ref_time']
        datum = copy.deepcopy(DATUM_TEMPLATE)
        datum['client_send'] = row['start'] + ref_time
        datum['server_recv'] = row[f"{ingest}_receive"] + ref_time
        datum['server_send'] = row[f"{ingest}_handle"] + ref_time
        datum['client_recv'] = row['end'] + ref_time
        data.append(datum)
    data = sorted(data, key=lambda datum: datum['server_recv'])
    ref_time = data[0]['server_recv']
    for datum in data:
        datum['client_send'] = datum['client_send'] - ref_time
        datum['server_recv'] = datum['server_recv'] - ref_time
        datum['server_send'] = datum['server_send'] - ref_time
        datum['client_recv'] = datum['client_recv'] - ref_time
    
    xs = [datum['server_recv'] / 1000 for datum in data]
    plt.plot(xs, [datum['server_send'] / 1000 for datum in data], label='server_send')
    plt.plot(xs, [datum['client_send'] / 1000 for datum in data], label='client_send')
    plt.plot(xs, [datum['client_recv'] / 1000 for datum in data], label='client_recv')
    plt.plot(xs, [datum['server_recv'] / 1000 for datum in data], label='server_recv')
    plt.legend()
    plt.show()
    """