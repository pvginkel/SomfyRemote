library('JenkinsPipelineUtils') _

withCredentials([
    string(credentialsId: 'WIFI_PASSWORD', variable: 'WIFI_PASSWORD'),
]) {
    podTemplate(inheritFrom: 'jenkins-agent-large', containers: [
        containerTemplate(name: 'idf', image: 'espressif/idf:v5.5.1', command: 'sleep', args: 'infinity', envVars: [
            containerEnvVar(key: 'WIFI_PASSWORD', value: '$WIFI_PASSWORD'),
        ])
    ]) {
        node(POD_LABEL) {
            stage('Build somfy remote') {
                dir('esp-libs') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/esp-libs.git'
                }

                dir('SomfyRemote') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/SomfyRemote.git'
                        
                    container('idf') {
                        // Necessary because the IDF container doesn't have support
                        // for setting the uid/gid.
                        sh 'git config --global --add safe.directory \'*\''
                        
                        // The Docker build isn't resolving the libraries from the
                        // relative folder. Not sure why. Instead it expects the
                        // components to be in the components folder.
                        sh 'mkdir -p components'
                        sh 'cp -a ../esp-libs/esp-support components'
                        sh 'cp -a ../esp-libs/esp-network-support components'

                        sh 'chmod +x scripts/dockerbuild.sh'
                        sh '/opt/esp/entrypoint.sh scripts/dockerbuild.sh'
                    }
                }
            }
            
            stage('Deploy somfy remote') {
                dir('HelmCharts') {
                    git branch: 'main',
                        credentialsId: '5f6fbd66-b41c-405f-b107-85ba6fd97f10',
                        url: 'https://github.com/pvginkel/HelmCharts.git'
                }

                dir('SomfyRemote') {
                    sh 'cp build/somfy-remote.bin somfy-remote-ota.bin'

                    sh 'chmod +x scripts/upload.sh'
                    sh 'scripts/upload.sh ../HelmCharts/assets/kubernetes-signing-key somfy-remote-ota.bin'
                }
            }
        }
    }
}
