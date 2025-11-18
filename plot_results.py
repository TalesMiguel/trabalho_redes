#!/usr/bin/env python3

import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt
import os
import glob

def parse_flow_monitor(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    total_throughput = 0
    total_delay_sum = 0
    total_rx_packets = 0
    total_tx_packets = 0
    total_lost_packets = 0
    
    for flow in root.findall('.//Flow'):
        try:
            tx_bytes = int(flow.get('txBytes', 0))
            rx_bytes = int(flow.get('rxBytes', 0))
            tx_packets = int(flow.get('txPackets', 0))
            rx_packets = int(flow.get('rxPackets', 0))
            lost_packets = int(flow.get('lostPackets', 0))
            
            time_first_rx_str = flow.get('timeFirstRxPacket', '0')
            time_last_rx_str = flow.get('timeLastRxPacket', '0')
            delay_sum_str = flow.get('delaySum', '0')
            
            time_first_rx = float(time_first_rx_str.replace('ns','').replace('+',''))
            time_last_rx = float(time_last_rx_str.replace('ns','').replace('+',''))
            delay_sum = float(delay_sum_str.replace('ns','').replace('+',''))
            
            duration = (time_last_rx - time_first_rx) / 1e9
            
            if duration > 0:
                total_throughput += (rx_bytes * 8) / (duration * 1e6)
            
            total_delay_sum += delay_sum
            total_rx_packets += rx_packets
            total_tx_packets += tx_packets
            total_lost_packets += lost_packets
            
        except (ValueError, AttributeError):
            continue
    
    if total_tx_packets > 0:
        packet_loss = (total_lost_packets / total_tx_packets) * 100
    else:
        packet_loss = 0
    
    if total_rx_packets > 0:
        avg_delay = (total_delay_sum / total_rx_packets) / 1e6
    else:
        avg_delay = 0
    
    return {
        'throughput': total_throughput,
        'packet_loss': packet_loss,
        'avg_delay': avg_delay
    }


def main():
    input_dir = '/home/tales/grad/redes/results'
    output_dir = '/home/tales/grad/redes/results/plots'
    
    os.makedirs(output_dir, exist_ok=True)
    
    xml_files = glob.glob(os.path.join(input_dir, 'flow_*.xml'))
    
    data = {
        'udp': {'static': {}, 'mobile': {}},
        'tcp': {'static': {}, 'mobile': {}},
        'mixed': {'static': {}, 'mobile': {}}
    }
    
    for xml_file in xml_files:
        filename = os.path.basename(xml_file)
        parts = filename.replace('flow_', '').replace('.xml', '').split('_')
        
        protocol = parts[0]
        mobility = parts[1]
        n_clients = int(parts[2])
        
        metrics = parse_flow_monitor(xml_file)
        data[protocol][mobility][n_clients] = metrics
    
    clients = sorted([1, 2, 4, 8, 16, 32])
    
    protocols = {
        'udp': 'UDP',
        'tcp': 'TCP',
        'mixed': 'UDP+TCP'
    }
    
    colors = {
        'udp': '#3498db',
        'tcp': '#e74c3c',
        'mixed': '#2ecc71'
    }
    
    markers = {
        'udp': 'o',
        'tcp': 's',
        'mixed': '^'
    }
    
    mobility_labels = {
        'static': 'Estático',
        'mobile': 'Móvel'
    }
    
    for mobility in ['static', 'mobile']:
        plt.figure(figsize=(10, 6))
        for protocol in ['udp', 'tcp', 'mixed']:
            throughputs = [data[protocol][mobility].get(n, {}).get('throughput', 0) for n in clients]
            plt.plot(clients, throughputs, marker=markers[protocol], color=colors[protocol], 
                    linewidth=2, markersize=8, label=protocols[protocol])
        
        plt.xlabel('Número de Clientes', fontsize=12, fontweight='bold')
        plt.ylabel('Vazão (Mbps)', fontsize=12, fontweight='bold')
        plt.title(f'Vazão x Número de Clientes ({mobility_labels[mobility]})', fontsize=14, fontweight='bold')
        plt.xticks(clients)
        plt.legend(loc='best', fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'vazao_{mobility}.png'), dpi=300, bbox_inches='tight')
        plt.close()
    
    for mobility in ['static', 'mobile']:
        plt.figure(figsize=(10, 6))
        for protocol in ['udp', 'tcp', 'mixed']:
            delays = [data[protocol][mobility].get(n, {}).get('avg_delay', 0) for n in clients]
            plt.plot(clients, delays, marker=markers[protocol], color=colors[protocol], 
                    linewidth=2, markersize=8, label=protocols[protocol])
        
        plt.xlabel('Número de Clientes', fontsize=12, fontweight='bold')
        plt.ylabel('Atraso (ms)', fontsize=12, fontweight='bold')
        plt.title(f'Atraso x Número de Clientes ({mobility_labels[mobility]})', fontsize=14, fontweight='bold')
        plt.xticks(clients)
        plt.legend(loc='best', fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'atraso_{mobility}.png'), dpi=300, bbox_inches='tight')
        plt.close()
    
    for mobility in ['static', 'mobile']:
        plt.figure(figsize=(10, 6))
        for protocol in ['udp', 'tcp', 'mixed']:
            losses = [data[protocol][mobility].get(n, {}).get('packet_loss', 0) for n in clients]
            plt.plot(clients, losses, marker=markers[protocol], color=colors[protocol], 
                    linewidth=2, markersize=8, label=protocols[protocol])
        
        plt.xlabel('Número de Clientes', fontsize=12, fontweight='bold')
        plt.ylabel('Pacotes Perdidos (%)', fontsize=12, fontweight='bold')
        plt.title(f'Perda de Pacotes x Número de Clientes ({mobility_labels[mobility]})', fontsize=14, fontweight='bold')
        plt.xticks(clients)
        plt.legend(loc='best', fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'perda_pacotes_{mobility}.png'), dpi=300, bbox_inches='tight')
        plt.close()
    
    print('Graficos gerados:')
    print(f'  - {os.path.join(output_dir, "vazao_static.png")}')
    print(f'  - {os.path.join(output_dir, "vazao_mobile.png")}')
    print(f'  - {os.path.join(output_dir, "atraso_static.png")}')
    print(f'  - {os.path.join(output_dir, "atraso_mobile.png")}')
    print(f'  - {os.path.join(output_dir, "perda_pacotes_static.png")}')
    print(f'  - {os.path.join(output_dir, "perda_pacotes_mobile.png")}')

if __name__ == '__main__':
    main()
