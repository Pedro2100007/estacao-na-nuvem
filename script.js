// Configurações do ThingSpeak
const channelID = '2555542';
const readAPIKey = '62NAEIAGZDOUMAX8';
const writeAPIKey = 'HM9V6JB10EKLMUZ8';

//const STATUS_CHANNEL_ID = '2533413';
//const STATUS_READ_API_KEY = '7ORUZSCMCUEUAQ3Z';
//const STATUS_TIMEOUT = 60000; // 60 segundos

// Variáveis de controle
let TempoUltComando= 0; //Pega o tempo quando escreve na "Função atualizar campo"
const TempoEspComando = 15000; // 15 segundos - intervalo de tempo para escrever na "Função atualizar campo"

// Elementos da interface
const elements = {
  temperatura1: document.getElementById('temperatura1'),
  temperatura2: document.getElementById('temperatura2'),
  pressao: document.getElementById('pressao'),
  altitude: document.getElementById('altitude'),
  umidade: document.getElementById('umidade'),
  chuva: document.getElementById('chuva'),
  statusTelhado: document.getElementById('statusTelhado'),
  abrir: document.getElementById('abrir'),
  fechar: document.getElementById('fechar'),
  telhadoIcon: document.getElementById('telhadoIcon'),
};

// Função para atualizar a interface HTML
function atualizaUI(data) {
  console.log("Dados recebidos para atualização:", data);
    
  if (data.field1 !== undefined && data.field1 !== null) {
    elements.temperatura1.textContent = parseFloat(data.field1).toFixed(1);
  }

  if (data.field4 !== undefined && data.field4 !== null) {
    elements.temperatura2.textContent = parseFloat(data.field4).toFixed(1);
  }
  
  if (data.field2 !== undefined && data.field2 !== null) {
    elements.pressao.textContent = parseFloat(data.field2).toFixed(1);
  }

  if (data.field3 !== undefined && data.field3 !== null) {
    elements.altitude.textContent = parseFloat(data.field3).toFixed(1);
  }

    if (data.field5 !== undefined && data.field5 !== null) {
    elements.umidade.textContent = parseFloat(data.field3).toFixed(1);
  }

    if (data.field6 !== undefined && data.field6 !== null) {
    elements.chuva.textContent = parseFloat(data.field3).toFixed(1);
  }
  
  if (data.field3 !== undefined && data.field3 !== null) {
    const telhadoState = parseInt(data.field3);
    elements.statusTelhado.textContent = telhadoState ? 'Ligado' : 'Desligado';
    elements.statusTelhado.className = telhadoState ? 'sensor-value on' : 'sensor-value off';
    elements.telhadoIcon.className = telhadoState ? 'sensor-icon pump on' : 'sensor-icon pump off';
  }
  

}

// Função para buscar dados do ThingSpeak
async function pegaDados() {
  try {
    const response = await fetch(`https://api.thingspeak.com/channels/${channelID}/feeds/last.json?api_key=${readAPIKey}`);
    
    if (!response.ok) throw new Error(`Erro HTTP: ${response.status}`);
    
    const data = await response.json();
    console.log("Dados completos da API:", data);
    
    if (!data || Object.keys(data).length === 0) {
      throw new Error("Dados vazios recebidos da API");
    }
    
    //chama função de atualizar HTML 
    atualizaUI(data);
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
        //const status = parseInt(data.field3);
        //if (status === 1) {
           // setStatus('AUTOMÁTICO', 'automatic');
       // } else if (status === 0) {
            //setStatus('MANUAL', 'manual');
        //} else {
            //setStatus('DESCONECTADO', 'disconnected');
        //}
        
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

// Função para atualizar campo no ThingSpeak
async function updateField(field, value) {
  const now = Date.now();
  if (now - TempoUltComando< TempoEspComando) {
    const waitTime = Math.ceil((TempoEspComando - (now - lastCommandTime))/1000);
    alert(`Aguarde ${waitTime} segundos antes de enviar outro comando.`);
    return false;
  }
  
  try {
    const url = `https://api.thingspeak.com/update?api_key=${writeAPIKey}&${field}=${value}`;
    const response = await fetch(url);
    
    if (!response.ok) throw new Error(`Erro HTTP: ${response.status}`);
    
    const result = await response.text();
    console.log(`Campo ${field} atualizado. Resposta:`, result);
    
    TempoUltComando= Date.now();
    await pegaDados(); // Atualiza a interface após mudança
    return true;
    
  } catch (error) {
    console.error(`Erro ao atualizar ${field}:`, error);
    return false;
  }
}

// Configuração dos event listeners da interface HTML
function setupEventListeners() {
  // Controles da bomba
  elements.abrir.addEventListener('click', () => {
    sendCommand('bomba', 1);
  });

  elements.fechar.addEventListener('click', () => {
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
    await pegaDados();
    
    // Atualização periódica
    setInterval(pegaDados, 5000);
    setInterval(updateSystemStatus, 5000);
    console.log("Configuração completa. Monitorando dados...");
});