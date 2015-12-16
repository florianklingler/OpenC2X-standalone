t <- read.delim("stats.csv", col.names=c("type", "seq", "delay"))

sent <- max(t$seq)+1
recv <- length(unique(t$seq))

barplot(recv/sent, ylim=c(0,1))
title(ylab="frac of received packets")
