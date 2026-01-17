#!/usr/bin/env node

import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { CallToolRequestSchema, ListToolsRequestSchema } from '@modelcontextprotocol/sdk/types.js';
import { spawn } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const PROJECT_ROOT = path.resolve(__dirname, '../../..');
const CONTROL_PIPE = path.join(PROJECT_ROOT, 'simulator_control.pipe');

class SimulatorControlServer {
  constructor() {
    this.server = new Server(
      {
        name: 'simulator-control',
        version: '1.0.0',
      },
      {
        capabilities: {
          tools: {},
        },
      }
    );

    this.setupToolHandlers();

    // Error handling
    this.server.onerror = (error) => console.error('[MCP Error]', error);
    process.on('SIGINT', async () => {
      await this.server.close();
      process.exit(0);
    });
  }

  setupToolHandlers() {
    this.server.setRequestHandler(ListToolsRequestSchema, async () => ({
      tools: [
        {
          name: 'send_command',
          description: 'Send a command to the running simulator. Commands: pause, resume, speed_up, speed_down, screenshot, set_nametag_size <value>, set_creature_detail <value>, reload_shaders, debug_info',
          inputSchema: {
            type: 'object',
            properties: {
              command: {
                type: 'string',
                description: 'Command to send to simulator',
              },
            },
            required: ['command'],
          },
        },
        {
          name: 'get_screenshot',
          description: 'Get the latest screenshot from the simulator',
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'get_debug_info',
          description: 'Get current debug information from simulator (FPS, creature count, render stats)',
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'adjust_rendering',
          description: 'Adjust rendering parameters in real-time',
          inputSchema: {
            type: 'object',
            properties: {
              parameter: {
                type: 'string',
                enum: ['nametag_size', 'creature_lod', 'animation_speed', 'lighting_ambient', 'lighting_diffuse'],
                description: 'Parameter to adjust',
              },
              value: {
                type: 'number',
                description: 'New value for the parameter',
              },
            },
            required: ['parameter', 'value'],
          },
        },
      ],
    }));

    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      switch (request.params.name) {
        case 'send_command':
          return await this.handleSendCommand(request.params.arguments);
        case 'get_screenshot':
          return await this.handleGetScreenshot();
        case 'get_debug_info':
          return await this.handleGetDebugInfo();
        case 'adjust_rendering':
          return await this.handleAdjustRendering(request.params.arguments);
        default:
          throw new Error(`Unknown tool: ${request.params.name}`);
      }
    });
  }

  async handleSendCommand(args) {
    const { command } = args;

    try {
      // Write command to control pipe
      const commandFile = path.join(PROJECT_ROOT, 'simulator_command.txt');
      fs.writeFileSync(commandFile, command + '\n');

      return {
        content: [
          {
            type: 'text',
            text: `Command sent: ${command}`,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Error sending command: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async handleGetScreenshot() {
    const screenshotPath = path.join(PROJECT_ROOT, 'screenshot.png');

    try {
      // Request screenshot
      const commandFile = path.join(PROJECT_ROOT, 'simulator_command.txt');
      fs.writeFileSync(commandFile, 'screenshot\n');

      // Wait for screenshot to be created
      await new Promise(resolve => setTimeout(resolve, 500));

      if (fs.existsSync(screenshotPath)) {
        const imageData = fs.readFileSync(screenshotPath, 'base64');
        return {
          content: [
            {
              type: 'image',
              data: imageData,
              mimeType: 'image/png',
            },
            {
              type: 'text',
              text: 'Screenshot captured',
            },
          ],
        };
      } else {
        return {
          content: [
            {
              type: 'text',
              text: 'Screenshot not yet available, try again in a moment',
            },
          ],
        };
      }
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Error getting screenshot: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async handleGetDebugInfo() {
    const debugFile = path.join(PROJECT_ROOT, 'debug_info.txt');

    try {
      // Request debug info
      const commandFile = path.join(PROJECT_ROOT, 'simulator_command.txt');
      fs.writeFileSync(commandFile, 'debug_info\n');

      // Wait for debug info to be written
      await new Promise(resolve => setTimeout(resolve, 200));

      if (fs.existsSync(debugFile)) {
        const debugInfo = fs.readFileSync(debugFile, 'utf8');
        return {
          content: [
            {
              type: 'text',
              text: `Debug Info:\n${debugInfo}`,
            },
          ],
        };
      } else {
        return {
          content: [
            {
              type: 'text',
              text: 'Debug info not yet available',
            },
          ],
        };
      }
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Error getting debug info: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async handleAdjustRendering(args) {
    const { parameter, value } = args;

    try {
      const commandFile = path.join(PROJECT_ROOT, 'simulator_command.txt');
      fs.writeFileSync(commandFile, `set ${parameter} ${value}\n`);

      return {
        content: [
          {
            type: 'text',
            text: `Set ${parameter} to ${value}`,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Error adjusting rendering: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async run() {
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
    console.error('Simulator Control MCP server running on stdio');
  }
}

const server = new SimulatorControlServer();
server.run().catch(console.error);
