const CronJobManager = require('cron-job-manager')
const fetch = require('node-fetch');
const { Client, GatewayIntentBits } = require('discord.js');
const { DISCORD_HIGHLIGHTS_HOOK, DISCORD_BOT_TOKEN } = require('../config');
const CONSTANTS = require('../constants');

const { Auction, Stats, User } = require('../models')

const guildId = CONSTANTS.DISCORD_GUILD_ID;
const topDtRoleId = CONSTANTS.DISCORD_ROLE_ID_DAMAGE_TITAN;
const topKdRoleId = CONSTANTS.DISCORD_ROLE_ID_CYBER_ASSASSIN;
const topAccRoleId = CONSTANTS.DISCORD_ROLE_ID_PRECISION_GHOST;

const client = new Client({ intents: [GatewayIntentBits.Guilds, GatewayIntentBits.GuildMembers] });
let guild = null;

client.once('clientReady', () => {  // Use 'once' for one-time setup; 'on' for ongoing
  console.log('Discord bot is ready');
  guild = client.guilds.cache.get(guildId);
  if (!guild) {
    console.error('Discord guild not found - Check if bot is in the server or ID is correct');
    return;
  }
  //console.log(`Discord guild found: ${guild.name} (${guild.id})`);
});

client.login(DISCORD_BOT_TOKEN).catch(console.error);

const jobManager = new CronJobManager()

jobManager.add('start job',
    '*/2 * * * *', // Every 1 min
    async function (){
        const now = Math.floor(Date.now() / 1000); // current time in seconds
        try {
            // Find all auctions that are not ended and have an endTime less than the current time
            const auctionsToEnd = await Auction.find({ ended: false, endTime: { $lt: now } });

            // Update each auction's ended status to true
            const updatePromises = auctionsToEnd.map(auction => {
                auction.ended = true;
                return auction.save();
            });
            await Promise.all(updatePromises);
        } catch (error) {
            console.error('Error checking and ending auctions:', error);
        }
    },
)

function getYesterday() {
  const now = new Date();
  now.setDate(now.getDate() - 1);
  return now.toISOString().split('T')[0];
}

async function getTop1(date, stat) {
  const pipeline = [
    { $unwind: '$dailyAchievements' },
    { $match: { 'dailyAchievements.date': date } },
    { $project: { userId: '$userId', userName: '$stats.userName', value: `$dailyAchievements.${stat}` } },
    { $sort: { value: -1 } },
    { $limit: 1 }
  ];
  const results = await Stats.aggregate(pipeline);
  return results.length > 0 ? results[0] : null;
}

async function postToDiscord(payload) {
  const webhookUrl = DISCORD_HIGHLIGHTS_HOOK;
  if (!webhookUrl) {
    console.error('Discord webhook URL not set');
    return;
  }
  try {
    // Log the payload for debugging (remove after testing)
    //console.log('Discord payload:', JSON.stringify(payload, null, 2));

    const response = await fetch(webhookUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    if (!response.ok) {
      console.error('Failed to post to Discord:', response.statusText);
    }
  } catch (error) {
    console.error('Error posting to Discord:', error);
  }
}

jobManager.add('dailyDiscordPost', '0 0 * * *', async () => {
  const date = getYesterday();
  //const date = '2025-09-08';
  console.log(`Running daily post for ${date}`);

  const topDt = await getTop1(date, 'dtRatio');
  const topKd = await getTop1(date, 'kdRatio');
  const topAcc = await getTop1(date, 'accPercent');

  if (!topDt && !topKd && !topAcc) {
    console.log(`No players on ${date}, skipping post`);
    return;
  }

  // Assign roles and collect level-up messages first
  const levelUps = [];
  const dtMsg = await assignRole(topDt, topDtRoleId, 'Damage Titan');
  if (dtMsg) levelUps.push(dtMsg);

  const kdMsg = await assignRole(topKd, topKdRoleId, 'Cyber Assassin');
  if (kdMsg) levelUps.push(kdMsg);

  const accMsg = await assignRole(topAcc, topAccRoleId, 'Precision Ghost');
  if (accMsg) levelUps.push(accMsg);

    const descriptions = [
    'In the neon shadows of the grid, these hackers dominated the circuits. Who will claim the throne tomorrow?',
    'Amid the flickering holograms of the sprawl, these cyber-samurai sliced through the competition. Will you jack in and seize the data crown next?',
    'In the underbelly of the megacity, where code flows like acid rain, these netrunners owned the shadows. Dare to challenge the matrix elite?',
    'Neon pulses in the void as these glitch gods crushed the opposition. The grid awaits its next overlord—plug in or fade out.',
    'From the chrome towers to the alleyway hacks, these digital phantoms ruled the night. Who\'s ready to overload the system tomorrow?',
    'Electric dreams shatter in the sprawl\'s haze; these console cowboys dominated the feed. Boot up and claim your slice of the infinite net.'
  ];

  const randomDescription = descriptions[Math.floor(Math.random() * descriptions.length)];

  // Cyberpunk color palette (hex values for neon vibes)
  const colors = [0x00FFEE, 0xFF00FF, 0xFF4500, 0x00FF00, 0xFFD700]; // Cyan, Magenta, Orange, Green, Gold
  const randomColor = colors[Math.floor(Math.random() * colors.length)];

  // Emoji sets for fields (rotate per category)
  const dtEmojis = ['🔥', '💥', '⚡'];
  const kdEmojis = ['💀', '🗡️', '☠️'];
  const accEmojis = ['🎯', '🔍', '🕶️'];

  const bonusTips = [
    'Pro Tip: Use your hand shield (right mouse button) to lower weapon damage.',
    'Pro Tip: Grapple to higher points by throwing your grapple (G button).',
    'Pro Tip: Stun your enemies by landing near them while flying.',
    'Pro Tip: Use your cyber-ghost item to render yourself invisible to enemies!',
    'Pro Tip: The Graviton weapon is very powerful; use it to suck enemies into a black hole!',
    'Pro Tip: Dashing (Y button) is a very powerful way to evade enemies.',
    'Pro Tip: Use the cyber-copy item to clone yourself and distract your enemies!',
    'Pro Tip: Use crouching (C button) to avoid making walking noises.',
    'Pro Tip: Keep moving to avoid snipers lurking on top of buildings.',
    'Pro Tip: Use the cyber-jump item to quickly jump onto high buildings!',
    'Pro Tip: Your score is heavily impacted by your K/D ratio.',
    'Pro Tip: To beat the AIs, fight them strategically (use flying, dashing, boost items).'
  ];
  const randomTip = bonusTips[Math.floor(Math.random() * bonusTips.length)];

  // Cyberpunk-themed embed template
  const embed = {
    title: `⚡ Neo-City Daily Overlords - ${date} 🌆`,
    description: randomDescription,
    color: randomColor, // Neon cyan for cyberpunk vibe (hex: #00FFEE)
    fields: [],
    footer: { text: 'Powered by Disruptive Labs - Hack the Planet' },
    timestamp: new Date().toISOString()
  };

  if (topKd && topKd.value > 0) {
    const randomKdEmoji = kdEmojis[Math.floor(Math.random() * kdEmojis.length)];
    embed.fields.push({
      name: `${randomKdEmoji} Cyber Assassin (kills/deaths ratio)`,
      value: `**${topKd.userName}**: ${topKd.value.toFixed(2)}`,
      inline: true
    });
  }

  if (topDt && topDt.value > 0) {
    const randomDtEmoji = dtEmojis[Math.floor(Math.random() * dtEmojis.length)];
    embed.fields.push({
      name: `${randomDtEmoji} Damage Titan (damage efficienty)`,
      value: `**${topDt.userName}**: ${topDt.value.toFixed(1)}%`,
      inline: true
    });
  }

  if (topAcc && topAcc.value > 0) {
    const randomAccEmoji = accEmojis[Math.floor(Math.random() * accEmojis.length)];
    embed.fields.push({
      name: `${randomAccEmoji} Precision Ghost (shooting accuracy)`,
      value: `**${topAcc.userName}**: ${topAcc.value.toFixed(2)}%`,
      inline: true
    });
  }

  if (levelUps.length > 0) {
    embed.fields.push({
      name: '',
      value: '\n',
      inline: false
    });
    embed.fields.push({
      name: '🌟 Grid Evolutions',
      value: levelUps.join('\n'),
      inline: false
    });
  }

  if (Math.random() > 0.5) {
    embed.fields.push({
      name: '',
      value: '\n',
      inline: false
    });
    embed.fields.push({
      name: '📡 Grid Transmission',
      value: randomTip,
      inline: false
    });
  }

  // Post the embed
  await postToDiscord({ embeds: [embed] });
  console.log(`Posted top performers for ${date}`);
});

async function assignRole(top, roleId, roleName) {

  if (!top || !guild) return null;
  const user = await User.findOne({ accountId: top.userId });
  if (!user || !user.discordId || !user.discordName || top.value == 0) return null;

  const discordId = user.discordId;
  const discordName = user.discordName;
  try {
    const member = await guild.members.fetch(discordId);
    if (member) {
      if (member.roles.cache.has(roleId) || discordId == "977865790147686420") {
        //console.log(`User ${discordName} (${discordId}) already has role ${roleId}`);
        return null;
      }
      await member.roles.add(roleId);
      //console.log(`Assigned role ${roleId} to ${discordName} (${discordId})`);
      return `🚀 **${discordName}** leveled up to **${roleName}** status in the sprawl! 🌃`; // Cyberpunk-themed message
    } else {
      //console.log(`Member ${discordId} not found in guild`);
      return null;
    }
  } catch (error) {
    //console.error(`Error assigning role to ${discordId}:`, error);
    return null;
  }
}

//jobManager.update('dailyDiscordPost', '*/1 * * * *')
jobManager.start('dailyDiscordPost');
jobManager.start('start job');
