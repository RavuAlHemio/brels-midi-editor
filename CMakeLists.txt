cmake_minimum_required(VERSION 3.10)

project(brels-midi-editor
    HOMEPAGE_URL "https://breno.sarmen.to/midi/"
    LANGUAGES C)

add_executable(midiedit WIN32
    brlsmidi.c
    inputbox.c
    mididec.c
    midiedit.c
    resources.rc)
target_link_libraries(midiedit
    comctl32.lib
    shlwapi.lib
    winmm.lib)

add_library(brlsmidi SHARED
    brlsmidi.c)
target_link_libraries(brlsmidi
    winmm.lib)

install(TARGETS midiedit brlsmidi
    RUNTIME DESTINATION ".")
install(FILES
    resources/abrir.ICO
    resources/ajuda.ICO
    resources/andamento.ICO
    resources/apagar.ICO
    resources/api.ICO
    resources/breve.ICO
    resources/buttons.txt
    resources/colar.ICO
    resources/colcheia.ICO
    resources/controllers.txt
    resources/copiar.ICO
    resources/copyright.ICO
    resources/cortar.ICO
    resources/decodificar.ICO
    resources/desconhecido.ICO
    resources/filtrar.ICO
    resources/fim.ICO
    resources/fusa.ICO
    resources/help.html
    resources/inicio.ICO
    resources/inserirtrilha.ICO
    resources/instrumento.ICO
    resources/instruments.txt
    resources/letra.ICO
    resources/logo.ico
    resources/logo48.ico
    resources/loop.ICO
    resources/marcador.ICO
    resources/minima.ICO
    resources/muda.ICO
    resources/nomedatrilha.ICO
    resources/nomedoinstrumento.ICO
    resources/novo.ICO
    resources/numerodatrilha.ICO
    resources/oitava_abaixo.ICO
    resources/oitava_acima.ICO
    resources/parar.ICO
    resources/pausar.ICO
    resources/pontodedica.ICO
    resources/removertrilha.ICO
    resources/salvar.ICO
    resources/salvarcomo.ICO
    resources/selecionar.ICO
    resources/selecionartudo.ICO
    resources/semibreve.ICO
    resources/semicolcheia.ICO
    resources/semifusa.ICO
    resources/seminima.ICO
    resources/sobre.ICO
    resources/solo.ICO
    resources/strings.txt
    resources/texto.ICO
    resources/tips.txt
    resources/tocar.ICO
    resources/transpor_abaixo.ICO
    resources/transpor_acima.ICO
    resources/version_history.txt
    DESTINATION ".")
