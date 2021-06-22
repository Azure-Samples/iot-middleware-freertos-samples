#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# This script sets up virtual interface (rtosveth1 and rtosveth0) with DHCP configure that can be used for linux port.

set -o errexit  # Exit if command failed.
set -o nounset  # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

getNextAvailableIpRange() {
    for (( i=1; i<=255; i++ )); do
        match=`ifconfig -a | grep -o -E "inet $1\.$2\.$i\.[0-9]+"`

        if [[ "$match" == "" ]]; then
            echo $i
            break
        fi
    done
}

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "Creates a virtual network interface for the freertos simulator."
    echo "Use option --clean to undo any changes."
elif [[ "$1" == "--clean" ]]; then
    ifconfig rtosveth0 down
    ifconfig rtosveth1 down
    ip link delete rtosveth0
    rm -f /etc/dhcp/dhcpd.conf
    rm -f /etc/default/isc-dhcp-server

    if [ -f "/etc/dhcp/dhcpd.conf.bkp" ]; then
        mv "/etc/dhcp/dhcpd.conf.bkp" "/etc/dhcp/dhcpd.conf"
    fi

    if [ -f "/etc/default/isc-dhcp-server.bkp" ]; then
        mv "/etc/default/isc-dhcp-server.bkp" "/etc/default/isc-dhcp-server"
    fi

    service isc-dhcp-server restart

    iptables-restore < /opt/iptables.backup
else
    if [ -f "/etc/dhcp/dhcpd.conf" ]; then
        cp -v /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.bkp
    fi

    if [ -f "/etc/default/isc-dhcp-server" ]; then
        cp -v "/etc/default/isc-dhcp-server" "/etc/default/isc-dhcp-server.bkp"
    fi

    ip_octet=$( getNextAvailableIpRange 192 168 )

    ip link add rtosveth0 type veth peer name rtosveth1
    ifconfig rtosveth0 192.168.$ip_octet.1 netmask 255.255.255.0 up
    ifconfig rtosveth1 up
    ethtool --offload rtosveth0 tx off
    ethtool --offload rtosveth1 tx off

    sh -c "cat > /etc/dhcp/dhcpd.conf" <<EOT
    subnet 192.168.$ip_octet.0 netmask 255.255.255.0 {
        range 192.168.$ip_octet.100 192.168.$ip_octet.200;
        option routers 192.168.$ip_octet.1;
        option domain-name-servers 1.1.1.1;
        option broadcast-address 192.168.$ip_octet.255;
        default-lease-time 600;
        max-lease-time 7200;
    }
EOT

    sh -c "cat > /etc/default/isc-dhcp-server" <<EOT
    INTERFACES="rtosveth0"
EOT
    service isc-dhcp-server restart

    iptables-save > /opt/iptables.backup

    sysctl net.ipv4.ip_forward=1
    iptables -F
    iptables -t nat -F
    iptables -t nat -A POSTROUTING -s 192.168.$ip_octet.0/24 -o eth0 -j MASQUERADE
    iptables -A FORWARD -d 192.168.$ip_octet.0/24 -o rtosveth0 -j ACCEPT
    iptables -A FORWARD -s 192.168.$ip_octet.0/24 -j ACCEPT

    #Debug
    ip link
    ip addr
    iptables -t nat -L -n
    cat /etc/dhcp/dhcpd.conf
    cat /etc/default/isc-dhcp-server
fi
