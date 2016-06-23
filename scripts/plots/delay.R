t <- read.delim("stats.csv", col.names=c("type", "seq", "delay"))

t$delay <- t$delay / (1000*1000)

plot.new()
plot.window(xlim=c(0,max(t$delay)), ylim=c(0,1))

e <- ecdf(t$delay)
x <- seq(0, max(t$delay), length.out=10000)
lines(x, e(x), col="black", lwd=2)
axis(1)
axis(2)
box()
title(xlab="delay in us", ylab="eCDF")
