// Configurações do ThingSpeak
const channelID = '2840207';
const readAPIKey = '5UWNQD21RD2A7QHG';
const writeAPIKey = '9NG6QLIN8UXLE2AH';

const STATUS_CHANNEL_ID = '2533413';
const STATUS_READ_API_KEY = '7ORUZSCMCUEUAQ3Z';
const STATUS_TIMEOUT = 60000; // 60 segundos

// Variáveis de controle
let lastCommandTime = 0;
const COMMAND_DELAY = 15000; // 15 segundos

// Elementos da interface
const elements = {
  temperature: document.getElementById('temperature'),
  level: document.getElementById('level'),
  statusBomba: document.getElementById('statusBomba'),
  statusAquecedor: document.getElementById('statusAquecedor'),
  bombaOn: document.getElementById('bombaOn'),
  bombaOff: document.getElementById('bombaOff'),
  pumpIcon: document.getElementById('pumpIcon'),
  heaterIcon: document.getElementById('heaterIcon')
};

// Função para atualizar a interface
function updateUI(data) {
  console.log("Dados recebidos para atualização:", data);
    
  if (data.field1 !== undefined && data.field1 !== null) {
    elements.temperature.textContent = parseFloat(data.field1).toFixed(1);
  }
  
  if (data.field2 !== undefined && data.field2 !== null) {
    elements.level.textContent = parseFloat(data.field2).toFixed(1);
  }
  
  if (data.field3 !== undefined && data.field3 !== null) {
    const bombaState = parseInt(data.field3);
    elements.statusBomba.textContent = bombaState ? 'Ligado' : 'Desligado';
    elements.statusBomba.className = bombaState ? 'sensor-value on' : 'sensor-value off';
    elements.pumpIcon.className = bombaState ? 'sensor-icon pump on' : 'sensor-icon pump off';
  }
  
  if (data.field4 !== undefined && data.field4 !== null) {
    const resistenciaState = parseInt(data.field4);
    elements.statusAquecedor.textContent = resistenciaState ? 'Ligado' : 'Desligado';
    elements.statusAquecedor.className = resistenciaState ? 'sensor-value on' : 'sensor-value off';
    elements.heaterIcon.className = resistenciaState ? 'sensor-icon heater on' : 'sensor-icon heater off';
  }
}

// Função para buscar dados do ThingSpeak
async function fetchData() {
  try {
    const response = await fetch(`https://api.thingspeak.com/channels/${channelID}/feeds/last.json?api_key=${readAPIKey}`);
    
    if (!response.ok) throw new Error(`Erro HTTP: ${response.status}`);
    
    const data = await response.json();
    console.log("Dados completos da API:", data);
    
    if (!data || Object.keys(data).length === 0) {
      throw new Error("Dados vazios recebidos da API");
    }
    
    updateUI(data);
    return data;
    
  } catch (error) {
    console.error("Erro ao buscar dados:", error);
    return null;
  }
}

// Função para atualizar o status do sistema
async function updateSystemStatus() {
    try {
        const response = await fetch(`https://api.thingspeak.com/channels/${STATUS_CHANNEL_ID}/feeds/last.json?api_key=${STATUS_READ_API_KEY}`);
        
        if (!response.ok) throw new Error(`Erro HTTP: ${response.status}`);
        
        const data = await response.json();
        const statusElement = document.getElementById('statusValue');
        const indicator = document.getElementById('systemStatus');
        
        if (!data || !data.created_at) {
            setStatus('DESCONECTADO', 'disconnected');
            return;
        }
        
        // Verifica se os dados estão atualizados
        const lastUpdate = new Date(data.created_at);
        const now = new Date();
        const diffSeconds = (now - lastUpdate) / 1000;
        
        if (diffSeconds > 30) {
            setStatus('DESCONECTADO', 'disconnected');
            return;
        }
        
        // Verifica o valor do field3 (0 = Manual, 1 = Automático)
        const status = parseInt(data.field3);
        if (status === 1) {
            setStatus('AUTOMÁTICO', 'automatic');
        } else if (status === 0) {
            setStatus('MANUAL', 'manual');
        } else {
            setStatus('DESCONECTADO', 'disconnected');
        }
        
        function setStatus(text, type) {
            statusElement.textContent = text;
            statusElement.className = `indicator-value ${type}`;
            
            // Altera a cor de fundo do indicador
            if (type === 'disconnected') {
                indicator.style.backgroundColor = 'rgba(231, 76, 60, 0.2)';
            } else {
                indicator.style.backgroundColor = 'rgba(46, 204, 113, 0.2)';
            }
        }
        
    } catch (error) {
        console.error("Erro ao verificar status:", error);
        const statusElement = document.getElementById('statusValue');
        const indicator = document.getElementById('systemStatus');
        statusElement.textContent = 'DESCONECTADO';
        statusElement.className = 'indicator-value disconnected';
        indicator.style.backgroundColor = 'rgba(231, 76, 60, 0.2)';
    }
}

// Função para atualizar um campo no ThingSpeak
async function updateField(field, value) {
  const now = Date.now();
  if (now - lastCommandTime < COMMAND_DELAY) {
    const waitTime = Math.ceil((COMMAND_DELAY - (now - lastCommandTime))/1000);
    alert(`Aguarde ${waitTime} segundos antes de enviar outro comando.`);
    return false;
  }
  
  try {
    const url = `https://api.thingspeak.com/update?api_key=${writeAPIKey}&${field}=${value}`;
    const response = await fetch(url);
    
    if (!response.ok) throw new Error(`Erro HTTP: ${response.status}`);
    
    const result = await response.text();
    console.log(`Campo ${field} atualizado. Resposta:`, result);
    
    lastCommandTime = Date.now();
    await fetchData(); // Atualiza a interface após mudança
    return true;
    
  } catch (error) {
    console.error(`Erro ao atualizar ${field}:`, error);
    return false;
  }
}

// Configuração dos event listeners
function setupEventListeners() {
  // Controles da bomba
  elements.bombaOn.addEventListener('click', () => {
    sendCommand('bomba', 1);
  });

  elements.bombaOff.addEventListener('click', () => {
    sendCommand('bomba', 0);
  });
}

// Função auxiliar para enviar comandos
async function sendCommand(type, value) {
  const field = type === 'bomba' ? 'field3' : 'field4';
  return await updateField(field, value);
}

// Inicialização
document.addEventListener('DOMContentLoaded', async () => {
    console.log("Página carregada. Iniciando configuração...");
    
    // Verificação dos elementos
    for (const [key, element] of Object.entries(elements)) {
        if (!element) console.error(`Elemento não encontrado: ${key}`);
    }
    
    setupEventListeners();
    
    // Primeiro verifica o status
    await updateSystemStatus();
    
    // Depois carrega os dados iniciais
    await fetchData();
    
    // Atualização periódica
    setInterval(fetchData, 5000);
    setInterval(updateSystemStatus, 5000);
    console.log("Configuração completa. Monitorando dados...");
});