# m3lilygo
software for lilygo t-watch to talk to mythric mystery master

This is super specific to a handful of people around the world, but if you run an escape room and use Mythric Mystery Master as a software and you buy a lilygo t-watch 2020, this code lets the t-watch talk directly to m3 either for the GM or the players to use.

The main things you need are an mqtt server (m3 can do this internally), a wifi network (set the wifi name and password in the code) and modify the story ID in the sub and pub settings to match your story (in this example they are AH.)

This is a pretty ropey way to use the graphics library, but I couldn't get the outcome I wanted from the lvgl library.  If somebody awesome with the lvgl library wants to improve it, i'd love to see how its properly done!
