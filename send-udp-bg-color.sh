#!/bin/bash
#
# Ceci est un exemple de commande UDP:
# l'arrière-plan est mis à jour en dégradé de teinte,
# bouclant indéfiniment selon le cercle des teintes.
#

value=255;
saturation=255
hue=0

inc=1

_signal() {
    echo "Got signal. Exiting..."
    exit 1
}

trap _signal SIGINT

while true; do
    hue=$(( (hue + inc) % 256 ))
    packet=:bg:hsl:$hue:$saturation:$value
    echo "Sending: $packet" >&2
    echo "$packet"
    sleep 0.2
done | socat - UDP-DATAGRAM:255.255.255.255:5555,broadcast
