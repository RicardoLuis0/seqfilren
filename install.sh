#!/usr/bin/env bash
while true; do
    read -p "Install for All users, Local user or Cancel (A/L/C)?" choice
    if [ "$choice" = "a" ] || [ "$choice" = "A" ]; then
        ln -srf bin/seqfilren /usr/local/bin/seqfilren;
        break;
    elif [ "$choice" = "l" ] || [ "$choice" = "L" ]; then
        ln -srf bin/seqfilren ~/bin/seqfilren;
        break;
    elif [ "$choice" = "c" ] || [ "$choice" = "C" ]; then
        break;
    else
      echo "invalid choice"
    fi
done
