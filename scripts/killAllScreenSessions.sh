screen -ls | grep "ached" | cut -d. -f1 | awk '{print $1}' | xargs kill
