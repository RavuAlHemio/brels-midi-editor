project('brels-midi-editor', 'c')
# https://breno.sarmen.to/midi/

windows = import('windows')

cc = meson.get_compiler('c')
comctl32 = cc.find_library('comctl32')
shlwapi = cc.find_library('shlwapi')
winmm = cc.find_library('winmm')

rc_args = []
if build_machine.system() != 'windows'
  rc_args = ['-DFORWARDSLASHES']
endif

resources_res = windows.compile_resources(
  'resources.rc',
  args: rc_args,
)
respkg_res = windows.compile_resources(
  'respkg.rc',
  args: rc_args,
)

executable(
  'midiedit',

  'brlsmidi.c',
  'inputbox.c',
  'mididec.c',
  'midiedit.c',

  resources_res,

  dependencies: [
    comctl32,
    shlwapi,
    winmm,
  ],

  win_subsystem: 'windows',
)

shared_library(
  'brlsmidi',

  'brlsmidi.c',

  dependencies: [
    winmm,
  ],
)

shared_library(
  'respkg',

  respkg_res,
)

install_data(
  'resources/abrir.ICO',
  'resources/ajuda.ICO',
  'resources/andamento.ICO',
  'resources/apagar.ICO',
  'resources/api.ICO',
  'resources/breve.ICO',
  'resources/buttons.txt',
  'resources/colar.ICO',
  'resources/colcheia.ICO',
  'resources/controllers.txt',
  'resources/copiar.ICO',
  'resources/copyright.ICO',
  'resources/cortar.ICO',
  'resources/decodificar.ICO',
  'resources/desconhecido.ICO',
  'resources/filtrar.ICO',
  'resources/fim.ICO',
  'resources/fusa.ICO',
  'resources/help.html',
  'resources/inicio.ICO',
  'resources/inserirtrilha.ICO',
  'resources/instrumento.ICO',
  'resources/instruments.txt',
  'resources/letra.ICO',
  'resources/logo.ico',
  'resources/logo48.ico',
  'resources/loop.ICO',
  'resources/marcador.ICO',
  'resources/minima.ICO',
  'resources/muda.ICO',
  'resources/nomedatrilha.ICO',
  'resources/nomedoinstrumento.ICO',
  'resources/novo.ICO',
  'resources/numerodatrilha.ICO',
  'resources/oitava_abaixo.ICO',
  'resources/oitava_acima.ICO',
  'resources/parar.ICO',
  'resources/pausar.ICO',
  'resources/pontodedica.ICO',
  'resources/removertrilha.ICO',
  'resources/salvar.ICO',
  'resources/salvarcomo.ICO',
  'resources/selecionar.ICO',
  'resources/selecionartudo.ICO',
  'resources/semibreve.ICO',
  'resources/semicolcheia.ICO',
  'resources/semifusa.ICO',
  'resources/seminima.ICO',
  'resources/sobre.ICO',
  'resources/solo.ICO',
  'resources/strings.txt',
  'resources/texto.ICO',
  'resources/tips.txt',
  'resources/tocar.ICO',
  'resources/transpor_abaixo.ICO',
  'resources/transpor_acima.ICO',
  'resources/version_history.txt',
)
