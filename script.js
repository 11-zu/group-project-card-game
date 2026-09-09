const suits = ["♥", "♠", "♣", "♦"];
const values = [1,2,3,4,5,6,7,8,9,10,11,12,13];

const playerCountSelect = document.getElementById('playerCount');
const startBtn = document.getElementById('startBtn');
const resetBtn = document.getElementById('resetBtn');
const playersEl = document.getElementById('players');
const logEl = document.getElementById('log');
const nameInputsEl = document.getElementById('nameInputs');
const turnPanel = document.getElementById('turnPanel');
const turnTitle = document.getElementById('turnTitle');
const turnMessage = document.getElementById('turnMessage');
const choiceArea = document.getElementById('choiceArea');
const actionBtn = document.getElementById('actionBtn');
const resultModal = document.getElementById('resultModal');
const resultContent = document.getElementById('resultContent');
const closeModalBtn = document.getElementById('closeModalBtn');

let gameState = null;
let currentTurn = null;
let activeSelection = null;

function buildNameInputs() {
  const count = Number(playerCountSelect.value);
  nameInputsEl.innerHTML = '';

  for (let i = 0; i < count; i++) {
    const box = document.createElement('div');
    box.className = 'name-box';
    box.innerHTML = `
      <label for="playerName${i + 1}">玩家 ${i + 1}</label>
      <input id="playerName${i + 1}" type="text" value="玩家${i + 1}" maxlength="12" />
    `;
    nameInputsEl.appendChild(box);
  }
}

function createDeck() {
  const deck = [];
  for (const suit of suits) {
    for (const value of values) {
      deck.push({ suit, value, label: `${suit}${value}` });
    }
  }
  return deck;
}

function shuffleDeck(deck) {
  for (let i = deck.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [deck[i], deck[j]] = [deck[j], deck[i]];
  }
}

function safeValue(card) {
  if (card.value === 1) return 1;
  if (card.value >= 11) return 10;
  return card.value;
}

function cardColor(suit) {
  return suit === '♥' || suit === '♦' ? 'red' : 'black';
}

function logMessage(msg) {
  const entry = document.createElement('div');
  entry.className = 'log-entry';
  entry.innerHTML = `<strong>日志：</strong> ${msg}`;
  logEl.prepend(entry);
}

function scoreText(score) {
  return score >= 0 ? `+${score}` : `${score}`;
}

function renderPlayers() {
  playersEl.innerHTML = '';

  if (!gameState || !gameState.players) {
    return;
  }

  gameState.players.forEach((player) => {
    const cardList = (player.hand || [])
      .map((card) => `<div class="card ${cardColor(card.suit)}">${card.label}</div>`)
      .join('');

    const block = document.createElement('div');
    block.className = 'player';
    block.innerHTML = `
      <div class="player-head">
        <span class="player-name">${player.name}</span>
        <span class="score-badge">分数：${scoreText(player.score)}</span>
      </div>
      <div class="hand">${cardList}</div>
    `;
    playersEl.appendChild(block);
  });
}

function selectLowestDiscard(player) {
  let bestIndex = 0;
  for (let i = 1; i < player.hand.length; i++) {
    if (player.hand[i].value < player.hand[bestIndex].value) {
      bestIndex = i;
    }
  }
  return bestIndex;
}

function compareCard(a, b) {
  if (a.value !== b.value) return a.value - b.value;
  return suits.indexOf(a.suit) - suits.indexOf(b.suit);
}

function round1(players) {
  logMessage('<strong>第 1 局</strong>：每人出一张牌，最小牌扣 1 分。');
  let minCard = players[0].hand[0];
  let minIndex = 0;

  players.forEach((player, index) => {
    const card = player.hand[0];
    logMessage(`${player.name} 出 ${card.label}`);
    if (compareCard(card, minCard) < 0) {
      minCard = card;
      minIndex = index;
    }
  });

  players[minIndex].score -= 1;
  logMessage(`${players[minIndex].name} 抽到最小牌 ${minCard.label}，扣 1 分。`);

  players.forEach((player, i) => {
    if (i !== minIndex) {
      player.hand.shift();
    }
  });

  players[minIndex].hand.shift();
}

function round2(players) {
  logMessage('<strong>第 2 局</strong>：每人出一张牌，最大牌扣 1 分。');
  let maxCard = players[0].hand[0];
  let maxIndex = 0;

  players.forEach((player, index) => {
    const card = player.hand[0];
    logMessage(`${player.name} 出 ${card.label}`);
    if (compareCard(card, maxCard) > 0) {
      maxCard = card;
      maxIndex = index;
    }
  });

  players[maxIndex].score -= 1;
  logMessage(`${players[maxIndex].name} 抽到最大牌 ${maxCard.label}，扣 1 分。`);

  players.forEach((player) => player.hand.shift());
}

function round3(players) {
  logMessage('<strong>第 3 局</strong>：21 点，每人出两张牌。');

  let bestIndex = -1;
  let bestTotal = -1;
  let validCount = 0;

  players.forEach((player, index) => {
    const first = player.hand[0];
    const second = player.hand[1];
    const total = safeValue(first) + safeValue(second);

    logMessage(`${player.name} 出 ${first.label} + ${second.label} = ${total}`);

    if (total <= 21) {
      validCount++;
      if (total > bestTotal) {
        bestTotal = total;
        bestIndex = index;
      }
    }
  });

  if (validCount === 0) {
    logMessage('本轮无人不超过 21，所有玩家扣 2 分。');
    players.forEach((player) => {
      player.score -= 2;
    });
  } else {
    const winner = players[bestIndex];
    logMessage(`${winner.name} 获胜，21 点结果为 ${bestTotal}。`);
    players.forEach((player, index) => {
      if (index !== bestIndex) {
        player.score -= 2;
      }
    });
  }

  players.forEach((player) => {
    player.hand.splice(0, 2);
  });
}

function round4(players) {
  logMessage('<strong>第 4 局</strong>：剩余三张牌比牌，最小总和扣 4 分。');

  let loserIndex = 0;
  let minSum = 999;

  players.forEach((player, index) => {
    const total = player.hand.slice(0, 3).reduce((sum, card) => sum + card.value, 0);
    logMessage(`${player.name} 三张牌总和是 ${total}`);
    if (total < minSum) {
      minSum = total;
      loserIndex = index;
    }
  });

  players[loserIndex].score -= 4;
  logMessage(`${players[loserIndex].name} 三张牌最小，扣 4 分。`);

  players.forEach((player) => {
    player.hand.splice(0, 3);
  });
}

function getPlayerNames(playerCount) {
  const names = [];
  for (let i = 0; i < playerCount; i++) {
    const input = document.getElementById(`playerName${i + 1}`);
    const value = input ? input.value.trim() : '';
    names.push(value || `玩家${i + 1}`);
  }
  return names;
}

function showResult(players) {
  const ranking = [...players].sort((a, b) => b.score - a.score);
  const winner = ranking[0];

  resultContent.innerHTML = `
    <div>冠军：<strong>${winner.name}</strong>，最终分数：<strong>${winner.score}</strong></div>
    <div>${ranking.map((p, i) => `${i + 1}. ${p.name}：${p.score}`).join('<br>')}</div>
  `;

  resultModal.classList.remove('hidden');
}

function sameCard(a, b) {
  if (!a || !b) return false;
  return a.value === b.value && a.suit === b.suit;
}

function removeChosenCards(player, chosenCards) {
  player.hand = player.hand.filter((card) => !chosenCards.some((chosen) => sameCard(card, chosen)));
}

function chooseAutoDiscard(player) {
  let best = player.hand[0];
  for (let i = 1; i < player.hand.length; i++) {
    if (player.hand[i].value < best.value) {
      best = player.hand[i];
    }
  }
  return best;
}

function getRoundChoices(roundNum, hand) {
  if (roundNum === 1 || roundNum === 2) {
    return hand.map((card) => [card]);
  }

  if (roundNum === 3) {
    const options = [];
    for (let i = 0; i < hand.length; i++) {
      for (let j = i + 1; j < hand.length; j++) {
        options.push([hand[i], hand[j]]);
      }
    }
    return options.length > 0 ? options : [[hand[0], hand[1]]];
  }

  const options = [];
  for (let i = 0; i < hand.length; i++) {
    for (let j = i + 1; j < hand.length; j++) {
      for (let k = j + 1; k < hand.length; k++) {
        options.push([hand[i], hand[j], hand[k]]);
      }
    }
  }
  return options.length > 0 ? options : [[hand[0], hand[1], hand[2]]];
}

function chooseAutoPlay(roundNum, hand) {
  const choices = getRoundChoices(roundNum, hand);

  if (roundNum === 1) {
    return choices.reduce((best, current) => (compareCard(current[0], best[0]) < 0 ? current : best), choices[0]);
  }

  if (roundNum === 2) {
    return choices.reduce((best, current) => (compareCard(current[0], best[0]) > 0 ? current : best), choices[0]);
  }

  if (roundNum === 3) {
    let best = choices[0];
    let bestTotal = safeValue(best[0]) + safeValue(best[1]);
    for (const option of choices) {
      const total = safeValue(option[0]) + safeValue(option[1]);
      if (total <= 21 && total > bestTotal) {
        best = option;
        bestTotal = total;
      }
    }
    return best;
  }

  return choices.reduce((best, current) => {
    const s1 = current.reduce((sum, card) => sum + card.value, 0);
    const s2 = best.reduce((sum, card) => sum + card.value, 0);
    return s1 < s2 ? current : best;
  }, choices[0]);
}

function showChoicePhase(playerIndex, title, message, choices) {
  currentTurn = { playerIndex, choices };
  activeSelection = null;
  turnTitle.textContent = title;
  turnMessage.textContent = message;
  choiceArea.innerHTML = '';

  choices.forEach((choice, index) => {
    const btn = document.createElement('button');
    btn.className = 'choice-card';
    btn.type = 'button';
    btn.textContent = Array.isArray(choice) ? choice.map((card) => card.label).join(' + ') : choice.label;
    btn.addEventListener('click', () => {
      activeSelection = index;
      document.querySelectorAll('.choice-card').forEach((el) => el.classList.remove('selected'));
      btn.classList.add('selected');
    });
    choiceArea.appendChild(btn);
  });

  turnPanel.classList.remove('hidden');
}

function runDiscardPhase() {
  if (!gameState) return;

  const players = gameState.players;
  if (gameState.currentPlayerIndex >= players.length) {
    gameState.currentPlayerIndex = 0;
    runRound(1);
    return;
  }

  const player = players[gameState.currentPlayerIndex];
  const choices = player.hand.map((card) => [card]);

  if (gameState.currentPlayerIndex === 0) {
    showChoicePhase(0, '弃牌阶段', `${player.name}，请选择一张你认为没用的牌弃掉。`, choices);
    actionBtn.onclick = () => {
      if (activeSelection === null || activeSelection === undefined) {
        logMessage(`${player.name} 还没有选择弃牌。`);
        return;
      }
      const selected = choices[activeSelection];
      removeChosenCards(player, selected);
      logMessage(`${player.name} 放弃了 ${selected[0].label}。`);
      gameState.currentPlayerIndex += 1;
      renderPlayers();
      runDiscardPhase();
    };
    return;
  }

  const discard = chooseAutoDiscard(player);
  removeChosenCards(player, [discard]);
  logMessage(`${player.name} 自动弃掉 ${discard.label}。`);
  gameState.currentPlayerIndex += 1;
  renderPlayers();
  runDiscardPhase();
}

function runRound(roundNum) {
  if (!gameState) return;

  const players = gameState.players;
  if (gameState.currentPlayerIndex >= players.length) {
    resolveRound(roundNum);
    return;
  }

  const player = players[gameState.currentPlayerIndex];
  const choices = getRoundChoices(roundNum, player.hand);

  if (gameState.currentPlayerIndex === 0) {
    const title = `第 ${roundNum} 局`;
    const message = roundNum === 1
      ? `${player.name}，请选择一张牌进行比大小。`
      : roundNum === 2
        ? `${player.name}，请选择一张牌进行比大小。`
        : roundNum === 3
          ? `${player.name}，请选择两张牌计算 21 点。`
          : `${player.name}，请选择三张牌参与比牌。`;

    showChoicePhase(gameState.currentPlayerIndex, title, message, choices);
    actionBtn.onclick = () => {
      if (activeSelection === null || activeSelection === undefined) {
        logMessage(`${player.name} 还没有选择牌。`);
        return;
      }
      player.selectedChoice = choices[activeSelection];
      logMessage(`${player.name} 选择了 ${choices[activeSelection].map((card) => card.label).join(' + ')}。`);
      gameState.currentPlayerIndex += 1;
      renderPlayers();
      runRound(roundNum);
    };
    return;
  }

  const autoChoice = chooseAutoPlay(roundNum, player.hand);
  player.selectedChoice = autoChoice;
  logMessage(`${player.name} 自动选择了 ${autoChoice.map((card) => card.label).join(' + ')}。`);
  gameState.currentPlayerIndex += 1;
  renderPlayers();
  runRound(roundNum);
}

function resolveRound(roundNum) {
  const players = gameState.players;
  const selected = players.map((player) => player.selectedChoice || [player.hand[0]]);

  if (roundNum === 1) {
    let minCard = selected[0][0];
    let minIndex = 0;
    for (let i = 1; i < selected.length; i++) {
      if (compareCard(selected[i][0], minCard) < 0) {
        minCard = selected[i][0];
        minIndex = i;
      }
    }
    players[minIndex].score -= 1;
    logMessage(`${players[minIndex].name} 这一局最小，扣 1 分。`);
  }

  if (roundNum === 2) {
    let maxCard = selected[0][0];
    let maxIndex = 0;
    for (let i = 1; i < selected.length; i++) {
      if (compareCard(selected[i][0], maxCard) > 0) {
        maxCard = selected[i][0];
        maxIndex = i;
      }
    }
    players[maxIndex].score -= 1;
    logMessage(`${players[maxIndex].name} 这一局最大，扣 1 分。`);
  }

  if (roundNum === 3) {
    let bestIndex = -1;
    let bestTotal = -1;
    for (let i = 0; i < selected.length; i++) {
      const total = safeValue(selected[i][0]) + safeValue(selected[i][1]);
      logMessage(`${players[i].name} 这局出 ${selected[i][0].label} + ${selected[i][1].label} = ${total}`);
      if (total <= 21 && total > bestTotal) {
        bestTotal = total;
        bestIndex = i;
      }
    }

    if (bestIndex === -1) {
      logMessage('本轮没有人不超过 21，所有玩家扣 2 分。');
      players.forEach((player) => {
        player.score -= 2;
      });
    } else {
      logMessage(`${players[bestIndex].name} 胜出 21 点，得分 ${bestTotal}，其他玩家扣 2 分。`);
      players.forEach((player, index) => {
        if (index !== bestIndex) player.score -= 2;
      });
    }
  }

  if (roundNum === 4) {
    let loserIndex = 0;
    let minSum = 999;
    for (let i = 0; i < selected.length; i++) {
      const total = selected[i].reduce((sum, card) => sum + card.value, 0);
      logMessage(`${players[i].name} 三张牌总和为 ${total}`);
      if (total < minSum) {
        minSum = total;
        loserIndex = i;
      }
    }
    players[loserIndex].score -= 4;
    logMessage(`${players[loserIndex].name} 这轮三张牌最小，扣 4 分。`);
  }

  players.forEach((player) => {
    removeChosenCards(player, player.selectedChoice || []);
    player.selectedChoice = null;
  });

  renderPlayers();
  gameState.currentPlayerIndex = 0;

  if (roundNum < 4) {
    runRound(roundNum + 1);
    return;
  }

  const ranking = [...players].sort((a, b) => a.score - b.score);
  logMessage('最终排名：' + ranking.map((p, i) => `${i + 1}.${p.name}(${p.score})`).join(' / '));
  showResult(players);
}

function startInteractiveGame() {
  logEl.innerHTML = '';

  const playerCount = Number(playerCountSelect.value);
  const names = getPlayerNames(playerCount);
  const deck = createDeck();
  shuffleDeck(deck);

  const players = Array.from({ length: playerCount }, (_, index) => ({
    name: names[index],
    score: 0,
    hand: [],
    selectedChoice: null
  }));

  for (let i = 0; i < 8; i++) {
    for (let p = 0; p < playerCount; p++) {
      players[p].hand.push(deck.shift());
    }
  }

  gameState = { players, currentPlayerIndex: 0 };
  renderPlayers();
  logMessage('游戏开始，发牌完成。');
  runDiscardPhase();
}

startBtn.addEventListener('click', startInteractiveGame);
resetBtn.addEventListener('click', initDemo);
closeModalBtn.addEventListener('click', () => {
  resultModal.classList.add('hidden');
  initDemo();
});
playerCountSelect.addEventListener('change', buildNameInputs);

function initDemo() {
  buildNameInputs();
  gameState = {
    players: [
      { name: '玩家1', score: 0, hand: [{ suit: '♥', value: 5, label: '♥5' }, { suit: '♠', value: 8, label: '♠8' }, { suit: '♣', value: 3, label: '♣3' }] },
      { name: '玩家2', score: 0, hand: [{ suit: '♦', value: 7, label: '♦7' }, { suit: '♠', value: 12, label: '♠12' }, { suit: '♥', value: 9, label: '♥9' }] },
      { name: '玩家3', score: 0, hand: [{ suit: '♣', value: 10, label: '♣10' }, { suit: '♦', value: 2, label: '♦2' }, { suit: '♠', value: 6, label: '♠6' }] },
      { name: '玩家4', score: 0, hand: [{ suit: '♥', value: 11, label: '♥11' }, { suit: '♣', value: 4, label: '♣4' }, { suit: '♦', value: 13, label: '♦13' }] }
    ]
  };
  renderPlayers();
  logEl.innerHTML = '';
  logMessage('欢迎来到纸牌对决，点击“开始游戏”开始一局。');
}

window.addEventListener('DOMContentLoaded', initDemo);
