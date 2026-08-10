/**
 * ESP32-S3 Field Station Dashboard
 * Multi-language support and Theme Toggling using Lucide and Flag-Icons.
 */

let client = null;
const statusEl = document.getElementById('connection-status');
const tableBody = document.getElementById('table-body');
const tableWrapper = document.querySelector('.table-wrapper');
const MQTT_TOPIC_DEFAULT = "sensores/+/dados";

// i18n Dictionary
const translations = {
    en: {
        title: "ESP32-S3 Telemetry",
        statusConnected: "Connected",
        statusDisconnected: "Disconnected",
        statusFailed: "Failed",
        "host-label": "Broker Host (IP):",
        "port-label": "Port (WS):",
        "connect-btn": "Connect",
        "last-measure-label": "Last measurement (ID):",
        "date-time-label": "Date / Time:",
        "card-temp": "Temperature",
        "card-hum": "Humidity",
        "card-lux": "Luminosity",
        "card-pres": "Pressure",
        "history-title": "History",
        "th-datetime": "Date/Time",
        "th-device": "Device ID",
        "th-ts": "Timestamp",
        "th-temp": "Temp [°C]",
        "th-hum": "Hum [%]",
        "th-lux": "Lum [Raw]",
        "th-pres": "Pres [hPa]",
        "clear-btn": "Clear History",
        "table-note": "Max 500 rows. Oldest are deleted.",
        none: "None",
        unknown: "Unknown"
    },
    pt: {
        title: "Telemetria ESP32-S3",
        statusConnected: "Conectado",
        statusDisconnected: "Desconectado",
        statusFailed: "Falha",
        "host-label": "Host do Broker (IP):",
        "port-label": "Porta (WS):",
        "connect-btn": "Conectar",
        "last-measure-label": "Última medição (ID):",
        "date-time-label": "Data / Hora:",
        "card-temp": "Temperatura",
        "card-hum": "Umidade",
        "card-lux": "Luminosidade",
        "card-pres": "Pressão",
        "history-title": "Histórico",
        "th-datetime": "Data/Hora",
        "th-device": "ID do dispositivo",
        "th-ts": "Timestamp",
        "th-temp": "Temp [°C]",
        "th-hum": "Umid [%]",
        "th-lux": "Lum [ADC]",
        "th-pres": "Pres [hPa]",
        "clear-btn": "Limpar Histórico",
        "table-note": "Máximo de 500 linhas. As mais antigas são excluídas.",
        none: "Nenhum",
        unknown: "Desconhecido"
    }
};

let currentLang = 'en';

function toggleTheme() {
    const isDark = document.body.classList.toggle('dark-theme');
    const themeBtn = document.getElementById('theme-btn');
    themeBtn.innerHTML = isDark ? '<i data-lucide="sun"></i>' : '<i data-lucide="moon"></i>';
    lucide.createIcons();
}

function setLanguage(lang) {
    currentLang = lang;
    const t = translations[lang];

    document.querySelector('h1').innerText = t.title;
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.getAttribute('data-i18n');
        el.innerText = t[key];
    });

    // Update flag icon using Circle-Flags CDN
    document.getElementById('lang-btn').innerHTML = lang === 'en'
        ? '<img src="https://hatscripts.github.io/circle-flags/flags/br.svg" width="28">'
        : '<img src="https://hatscripts.github.io/circle-flags/flags/us.svg" width="28">';

    // Update units and placeholders
    document.getElementById('lux-unit').innerText = lang === 'pt' ? 'ADC' : 'Raw';

    if (statusEl.classList.contains('connected')) statusEl.innerText = t.statusConnected;
    if (statusEl.classList.contains('disconnected')) statusEl.innerText = t.statusDisconnected;

    const deviceIdEl = document.getElementById('device-id');
    if (deviceIdEl.innerText === translations.en.none || deviceIdEl.innerText === translations.pt.none) {
        deviceIdEl.innerText = t.none;
    }
}

// Elementos das Métricas
const elTemp = document.getElementById('temp-val');
const elHum = document.getElementById('hum-val');
const elLux = document.getElementById('lux-val');
const elPres = document.getElementById('pres-val');
const elDevice = document.getElementById('device-id');
const elTime = document.getElementById('last-update');

const MAX_ROWS = 500;

function addRow(data, dateObj) {
    while (tableBody.children.length >= MAX_ROWS) {
        tableBody.removeChild(tableBody.firstElementChild);
    }

    const row = document.createElement('tr');
    const timeStr = dateObj.toLocaleString(currentLang === 'pt' ? 'pt-BR' : 'en-US');

    row.innerHTML = `
        <td>${timeStr}</td>
        <td>${data.timestamp || '--'}</td>
        <td>${data.device_id || '??'}</td>
        <td>${data.metrics?.temperature?.toFixed(2) || '0.00'}</td>
        <td>${data.metrics?.pressure?.toFixed(2) || '0.00'}</td>
        <td>${data.metrics?.relative_humidity?.toFixed(2) || '0.00'}</td>
        <td>${data.metrics?.luminosity || '0'}</td>
    `;

    tableBody.appendChild(row);
    tableWrapper.scrollTop = tableWrapper.scrollHeight;
}

function connect() {
    if (typeof AUTO_CONFIG === 'undefined') {
        console.error("Configuração não encontrada! Execute o setup.ps1.");
        statusEl.innerText = "Config Error";
        return;
    }

    const host = AUTO_CONFIG.host;
    const port = AUTO_CONFIG.port;
    const clientId = "web_client_" + Math.random().toString(16).substr(2, 8);

    client = new Paho.MQTT.Client(host, port, clientId);

    client.onConnectionLost = (res) => {
        statusEl.innerText = translations[currentLang].statusDisconnected;
        statusEl.className = "status disconnected";
    };

    client.onMessageArrived = (message) => {
        try {
            const data = JSON.parse(message.payloadString);

            if (data.metrics) {
                if (data.metrics.temperature !== undefined) elTemp.innerText = data.metrics.temperature.toFixed(2);
                if (data.metrics.relative_humidity !== undefined) elHum.innerText = data.metrics.relative_humidity.toFixed(2);
                if (data.metrics.luminosity !== undefined) elLux.innerText = data.metrics.luminosity;
                if (data.metrics.pressure !== undefined) elPres.innerText = data.metrics.pressure.toFixed(2);
            }

            elDevice.innerText = data.device_id || translations[currentLang].unknown;

            // Tratamento robusto de timestamp
            let displayDate;
            if (data.timestamp && data.timestamp > 1000000) {
                // Se o timestamp for muito grande (ms), converte para segundos
                const ts = data.timestamp > 20000000000 ? data.timestamp / 1000 : data.timestamp;
                displayDate = new Date(ts * 1000);
            } else {
                displayDate = new Date(); // Fallback para hora local do navegador
            }
            elTime.innerText = displayDate.toLocaleString(currentLang === 'pt' ? 'pt-BR' : 'en-US');

            addRow(data, displayDate);
        } catch (e) {
            console.error("JSON Error:", e);
        }
    };

    const options = {
        onSuccess: () => {
            statusEl.innerText = translations[currentLang].statusConnected;
            statusEl.className = "status connected";
            client.subscribe(MQTT_TOPIC_DEFAULT);
        },
        onFailure: (err) => {
            statusEl.innerText = translations[currentLang].statusFailed;
            statusEl.className = "status disconnected";
        }
    };
    client.connect(options);
}

// Inicialização
document.addEventListener('DOMContentLoaded', () => {
    // Definir ícone inicial do tema baseado na classe do body
    const isDark = document.body.classList.contains('dark-theme');
    document.getElementById('theme-btn').innerHTML = isDark ? '<i data-lucide="sun"></i>' : '<i data-lucide="moon"></i>';

    lucide.createIcons();
    connect(); // Conecta automaticamente usando AUTO_CONFIG
});

document.getElementById('lang-btn').addEventListener('click', () => {
    setLanguage(currentLang === 'en' ? 'pt' : 'en');
});

document.getElementById('theme-btn').addEventListener('click', toggleTheme);

document.getElementById('clear-btn').addEventListener('click', () => {
    tableBody.innerHTML = '';
});
